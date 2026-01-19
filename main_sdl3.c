#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>

#define LOG_INFO(args...) SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, args)
#define LOG_WARN(args...) SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, args)
#define LOG_ERROR(args...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, args)
#define LOG_DEBUG(args...) SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, args)

#if DEBUG
#define DEFAULT_LOG_LEVEL SDL_LOG_PRIORITY_DEBUG
#else
#define DEFAULT_LOG_LEVEL SDL_LOG_PRIORITY_INFO
#endif

#define MAX_TIMESTEP_MS 100
#define INACTIVITY_TIME_MS 2000

static SDL_FColor s_colours[] = {
    {0.0f, 0.0f, 0.0f, SDL_ALPHA_OPAQUE_FLOAT},
    {1.0f, 0.0f, 0.0f, SDL_ALPHA_OPAQUE_FLOAT},
    {0.0f, 1.0f, 0.0f, SDL_ALPHA_OPAQUE_FLOAT},
    {0.0f, 0.0f, 1.0f, SDL_ALPHA_OPAQUE_FLOAT},
    {1.0f, 1.0f, 1.0f, SDL_ALPHA_OPAQUE_FLOAT},
};

typedef struct {
    bool quit_on_focus_lost;
} config_t;

typedef struct {
    size_t prev_colour_i;
    size_t target_colour_i;
    float time;
} render_state_t;

inline static float flerp(float a, float b, float t) {
    return a * (1.0f - t) + b * t;
}

int main(int argc_, char** argv_) {
    size_t argc = (size_t)argc_;
    const char** argv = (const char**)argv_;

    SDL_SetLogPriorityPrefix(SDL_LOG_PRIORITY_INFO, "INFO: ");
    SDL_SetLogPriorityPrefix(SDL_LOG_PRIORITY_TRACE, "TRACE: ");
    SDL_SetLogPriorityPrefix(SDL_LOG_PRIORITY_DEBUG, "DEBUG: ");
    SDL_SetLogPriorityPrefix(SDL_LOG_PRIORITY_VERBOSE, "VERBOSE: ");
    SDL_LogPriority log_level = DEFAULT_LOG_LEVEL;
    config_t config;
    config.quit_on_focus_lost = true;

    for (size_t i = 1; i < argc; i++) {
        const char* arg = argv[i];

        if (arg[0] != '-') {
            LOG_WARN("unknown argument: %s", arg);
            continue;
        }

        if (strncmp("-l", arg, 2) == 0 ||
            strncmp("--log-level", arg, sizeof("--log-level")) == 0) {
            if (++i >= argc) {
                LOG_ERROR("missing argument for %s", arg);
                return EXIT_FAILURE;
            }
            arg = argv[i];
            if (strncmp("info", arg, 4) == 0)
                log_level = SDL_LOG_PRIORITY_INFO;
            else if (strncmp("warn", arg, 4) == 0)
                log_level = SDL_LOG_PRIORITY_WARN;
            else if (strncmp("error", arg, 5) == 0)
                log_level = SDL_LOG_PRIORITY_ERROR;
            else if (strncmp("debug", arg, 5) == 0)
                log_level = SDL_LOG_PRIORITY_DEBUG;
            else if (strncmp("verbose", arg, 7) == 0)
                log_level = SDL_LOG_PRIORITY_VERBOSE;
            else if (strncmp("trace", arg, 5) == 0)
                log_level = SDL_LOG_PRIORITY_TRACE;
            else {
                LOG_ERROR("invalid log level provided: %s", arg);
                return EXIT_FAILURE;
            }

            continue;
        }
        if (strncmp("-f", arg, 2) == 0 ||
            strncmp("--ignore-focus", arg, sizeof("--ignore-focus")) == 0) {
            config.quit_on_focus_lost = false;
            LOG_INFO("disabled quit on focus lost");
            continue;
        }

        LOG_ERROR("unknown flag: %s", arg);
        return EXIT_FAILURE;
    }

    SDL_SetLogPriorities(log_level);

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    if (!SDL_SetAppMetadata("blanker", BUILD_HASH, "com.kareigu.blanker")) {
        LOG_WARN("Failed setting metadata: %s", SDL_GetError());
    }

    if (!SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1")) {
        LOG_WARN("Failed setting : %s", SDL_GetError());
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        LOG_ERROR("Failed initialising SDL: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    if (!SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_LOW)) {
        LOG_WARN("Failed setting thread priority to low: %s", SDL_GetError());
    }

    const SDL_DisplayMode* display_mode =
        SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
    if (display_mode == NULL) {
        SDL_Log("Failed getting primary display info: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    size_t window_width = display_mode->w * display_mode->pixel_density;
    size_t window_height = display_mode->h * display_mode->pixel_density;
    LOG_INFO("Setting window size to %lux%lu", window_width, window_height);
    if (!SDL_CreateWindowAndRenderer("blanker", window_width, window_height,
                                     SDL_WINDOW_BORDERLESS |
                                         SDL_WINDOW_FULLSCREEN,
                                     &window, &renderer)) {
        LOG_ERROR("Failed creating window or renderer: %s", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, 1, 1,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    if (!SDL_SetRenderVSync(renderer, 1)) {
        LOG_WARN("Failed enabling vsync: %s", SDL_GetError());
    }

    uint64_t target_loop_duration_ms = 16;
    if (display_mode->refresh_rate > 0.0f) {
        target_loop_duration_ms = 1000 / (uint64_t)display_mode->refresh_rate;
        LOG_INFO(
            "set target_loop_duration_ms from monitor refresh rate: %lu ms",
            target_loop_duration_ms);
    } else {
        LOG_WARN("Monitor has an invalid refresh rate, using default "
                 "target_loop_duration_ms = %lu",
                 target_loop_duration_ms);
    }

    if (!SDL_HideCursor()) {
        LOG_WARN("Failed hiding cursor: %s", SDL_GetError());
    }

    bool running = true;
    bool redraw = true;
    uint64_t prev_frame = SDL_GetTicks();
    uint64_t prev_action = prev_frame;
    uint64_t loop_duration_ms = target_loop_duration_ms;
    size_t current_colour = 0;
    render_state_t render_state = (render_state_t){
        .prev_colour_i = 0,
        .target_colour_i = 0,
        .time = 0.0f,
    };
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                if (config.quit_on_focus_lost) {
                    LOG_DEBUG("focus lost, exiting");
                    running = false;
                    continue;
                }
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                redraw = true;
                break;
            case SDL_EVENT_KEY_DOWN:
                switch (ev.key.key) {
                case SDLK_ESCAPE:
                case SDLK_Q:
                    LOG_DEBUG("exiting through keypress");
                    running = false;
                    continue;
                    break;
                case SDLK_R:
                    LOG_DEBUG("rerender triggered");
                    redraw = true;
                    break;
                case SDLK_SPACE: {
                    size_t new_colour = (size_t)SDL_rand(sizeof(s_colours) /
                                                         sizeof(SDL_FColor));
                    while (new_colour == current_colour)
                        new_colour = (size_t)SDL_rand(sizeof(s_colours) /
                                                      sizeof(SDL_FColor));

                    LOG_DEBUG("colour rerolled to %lu", new_colour);
                    current_colour = new_colour;
                    redraw = true;
                } break;
                }
            }
        }

        const uint64_t curr_frame = SDL_GetTicks();
        const uint64_t time_since = curr_frame - prev_frame;
        prev_frame = curr_frame;

        if (redraw) {
            loop_duration_ms = target_loop_duration_ms;
            const float animation_length_ms = 300.0f;
            prev_action = curr_frame;

            const SDL_FColor prev_colour = s_colours[render_state.prev_colour_i];
            const SDL_FColor target_colour = s_colours[render_state.target_colour_i];
            const float t = render_state.time / animation_length_ms;
            const float eased_t = -(SDL_cosf(3.1456 * t) - 1) / 2.0f;

            const SDL_FColor colour = {
                .r = flerp(prev_colour.r, target_colour.r, eased_t),
                .g = flerp(prev_colour.g, target_colour.g, eased_t),
                .b = flerp(prev_colour.b, target_colour.b, eased_t),
                .a = flerp(prev_colour.a, target_colour.a, eased_t),
            };
            SDL_SetRenderDrawColorFloat(renderer, colour.r, colour.g, colour.b,
                                        colour.a);
            SDL_RenderClear(renderer);
            SDL_RenderPresent(renderer);

            render_state.time += time_since;
            if (render_state.time > animation_length_ms) {
                render_state.time = 0.0f;
                render_state.prev_colour_i = render_state.target_colour_i;
                if (render_state.target_colour_i == current_colour)
                    redraw = false;
                else
                    render_state.target_colour_i = current_colour;
            }

            continue;
        }

        const uint64_t time_since_inactive = curr_frame - prev_action;
        if (time_since_inactive > INACTIVITY_TIME_MS &&
            loop_duration_ms < MAX_TIMESTEP_MS) {
            float t =
                (float)(time_since_inactive - INACTIVITY_TIME_MS) / 1000.0f;
            float new_target_f = (float)loop_duration_ms * (1.0f - t) +
                                 (float)MAX_TIMESTEP_MS * t;
            uint64_t new_target = (uint64_t)SDL_floorf(new_target_f);
            loop_duration_ms =
                new_target < MAX_TIMESTEP_MS ? new_target : MAX_TIMESTEP_MS;
            LOG_DEBUG("inactive: set tickrate to %lu ms", loop_duration_ms);
        }

        int wait_for = loop_duration_ms - time_since;
        if (wait_for <= 0) {
            continue;
        }
        SDL_Delay(wait_for);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
