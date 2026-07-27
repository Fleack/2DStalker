#include "Application.hpp"
#include "shared/logger/logger.hpp"

#include <exception>

int main()
{
    try
    {
        s2d::client::Application application;
        application.run();
        return 0;
    }
    catch (std::exception const& exception)
    {
        LOG(critical, "Client application failed: {}", exception.what());
    }
    catch (...)
    {
        LOG(critical, "Client application failed with an unknown error");
    }
    return 1;
}
