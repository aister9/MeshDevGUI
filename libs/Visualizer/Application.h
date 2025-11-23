#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <type_traits>
#include <algorithm>

#include "Stage.h"

namespace SPIN
{
    namespace Visualizer
    {
        class Application
        {
        public:
            virtual ~Application() = default;

            virtual void initialize(const std::vector<std::string>& /*args*/) {}
            virtual void start(Stage& stage) = 0;
            virtual void shutdown(Stage& /*stage*/) {}

            virtual const char* title() const { return "SPIN Visualizer Application"; }
            virtual int width() const { return 1600; }
            virtual int height() const { return 900; }
            virtual ImVec4 initialClearColor() const { return ImVec4(0.07f, 0.07f, 0.09f, 1.0f); }
        };

        template <typename TApplication>
        int launch(int argc, char** argv)
        {
            static_assert(std::is_base_of<Application, TApplication>::value,
                          "launch<TApplication>() requires TApplication to derive from Application");

            std::vector<std::string> args;
            args.reserve(static_cast<size_t>(std::max(argc - 1, 0)));
            for (int i = 1; i < argc; ++i)
            {
                if (argv[i])
                    args.emplace_back(argv[i]);
            }

            try
            {
                TApplication app;
                app.initialize(args);

                Stage stage(app.title(), app.width(), app.height());
                stage.setClearColor(app.initialClearColor());

                app.start(stage);
                stage.show();

                app.shutdown(stage);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Application launch failed: " << e.what() << std::endl;
                return EXIT_FAILURE;
            }
            catch (...)
            {
                std::cerr << "Application launch failed: unknown error" << std::endl;
                return EXIT_FAILURE;
            }

            return EXIT_SUCCESS;
        }
    }
}
