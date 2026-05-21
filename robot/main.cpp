#include <Core/Core.h>

#include <Vulkan/ApplicationBuilder.h>

#include "ApplicationState.h"

using namespace ref;
using namespace ref::vulkan;

void ConfigureShaders(ref::vulkan::Application& application)
{
    std::filesystem::path cache = "RefCache";
    auto& options = application.GetShaderLibrary().ModifyCompilationOptions();

#ifdef NDEBUG
    cache /= "Release";
    options.Optimization = ref::vulkan::OptimizationMode::Performance;
    options.GenerateDebugInfo = false;
    options.MacroDefinitions.push_back("REF_SHADER_RELEASE");
#else
    cache /= "Debug";
    options.Optimization = ref::vulkan::OptimizationMode::None;
    options.GenerateDebugInfo = true;
    options.MacroDefinitions.push_back("REF_SHADER_DEBUG");
#endif

    application.GetShaderLibrary().SetShaderCachePath(cache / "Shaders");
    application.GetPipelineLibrary().SetPipelineCachePath(cache);
    application.GetShaderLibrary().AddShadersFromDirectory("Shaders");
}

int main()
{
    logger::set_level(logger::level::debug);

    ApplicationBuilder::InitSystems();

    ApplicationBuilder builder;
    builder.EnableBase();
    builder.EnableFeatures(vk::PhysicalDeviceFeatures2().features.setGeometryShader(vk::True));
    builder.EnableFeatures(vk::PhysicalDeviceVulkan13Features().setShaderDemoteToHelperInvocation(vk::True));

    {
        vulkan::Application application = builder.CreateApplication("REF");
        ConfigureShaders(application);

        vulkan::ErrorApplicationState::AddToApplication(application);
        vulkan::CompilingShadersApplicationState::AddToApplication(application, "Robot State");

        application.AddAndCreateState<RobotApplicationState>("Robot State");

        application.Run(vulkan::CompilingShadersApplicationState::g_StateName);

        application.GetShaderLibrary().PruneShaderCache();
    }

    ApplicationBuilder::ShutdownSystems();

    return EXIT_SUCCESS;
}