#include "../builder/Application.h"

#include "cxxopts/cxxopts.hpp"

using namespace WorldAssistant;

int main(int argc, char *argv[])
{
    cxxopts::Options options("PackageTool", "A tool for building navigation data");
    options.add_options()
        ("h,help", "Print help and exit.")
        ("o,output", "Output directory.", cxxopts::value<std::string>())
        ("g,game", "GTA:SA folder.", cxxopts::value<std::string>())
        ;

    const auto result = options.parse(argc, argv);
    if (result.count("help"))
    {
        std::cout << options.help({""}) << std::endl;
        exit(0);
    }

    ApplicationParameters parameters;
    parameters.gamePath_ = result["game"].as<std::string>();
    parameters.outputPath_ = result["output"].as<std::string>();

    Application app(parameters);
    app.Run();

    return 0;
}
