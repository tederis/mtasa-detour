#pragma once

#include <filesystem>

#include "../scene/World.h"
#include "../builder/Game.h"

namespace WorldAssistant
{

struct ApplicationParameters
{
	std::filesystem::path gamePath_;
	std::filesystem::path outputPath_;
};

class Application
{
public:
	explicit Application(const ApplicationParameters& params) noexcept;

	bool Run();

private:
	ApplicationParameters params_;

	std::unique_ptr<Game> game_;
};

}
