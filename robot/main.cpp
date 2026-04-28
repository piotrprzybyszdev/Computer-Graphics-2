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
    builder.EnableFeatures(vk::PhysicalDeviceFeatures2().features.setGeometryShader(vk::True));
    builder.EnableFeatures(vk::PhysicalDeviceVulkan13Features().setShaderDemoteToHelperInvocation(vk::True));

    {
        vulkan::Application application = builder.CreateApplication("REF");
        application.AddAndCreateState<RobotApplicationState>("Robot State");
        application.Run("Robot State");
    }

    ApplicationBuilder::ShutdownSystems();
}