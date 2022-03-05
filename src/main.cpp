#include <iostream>

#include <context.hpp>
#include <memory>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <string>

using namespace polar;

int main(int argc, char** argv)
{
    try
    {
        Context::Param param{};
        param.enableCallback   = true;
        param.enableValidation = true;

        const Context context(param);
    }
    catch (const std::exception& excp)
    {
        spdlog::error(fmt::format("Caught exception when creating context: {}", excp.what()));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}