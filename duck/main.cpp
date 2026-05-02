#include <Core/Core.h>

#include <Vulkan/ApplicationBuilder.h>

#include "ApplicationState.h"

using namespace ref;
using namespace ref::vulkan;

int main()
{
    logger::set_level(logger::level::debug);

    ApplicationBuilder::InitSystems();

    ApplicationBuilder builder;
    builder.EnableBase();

    {
        vulkan::Application application = builder.CreateApplication("REF");
        application.GetShaderLibrary().SetShaderCachePath("ShaderCache");
        application.GetShaderLibrary().AddShadersFromDirectory("Shaders");

        vulkan::ErrorApplicationState::AddToApplication(application);
        vulkan::CompilingShadersApplicationState::AddToApplication(application, "Robot State");

        application.AddAndCreateState<DuckApplicationState>("Robot State");

        application.Run(vulkan::CompilingShadersApplicationState::g_StateName);

        application.GetShaderLibrary().PruneShaderCache();
    }

    ApplicationBuilder::ShutdownSystems();

    return EXIT_SUCCESS;
}