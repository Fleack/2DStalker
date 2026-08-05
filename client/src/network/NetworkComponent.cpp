#include "NetworkComponent.hpp"

#include "shared/logger/logger.hpp"

#include <exception>
#include <future>
#include <utility>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_future.hpp>

namespace
{
std::string exceptionMessage(std::exception_ptr error)
{
    try
    {
        std::rethrow_exception(error);
    }
    catch (std::exception const& exception)
    {
        return exception.what();
    }
    catch (...)
    {
        return "Unknown client error";
    }
}
} // namespace

namespace s2d::client
{
NetworkComponent::NetworkComponent(network::Config config)
    : m_work{boost::asio::make_work_guard(m_io)}
    , m_client{network::Client::create(m_io, std::move(config))}
    , m_worker{[this] {
        m_io.run();
    }}
{
}

NetworkComponent::~NetworkComponent()
{
    try
    {
        auto shutdown = boost::asio::co_spawn(
            m_io,
            m_client->disconnect(),
            boost::asio::use_future);

        shutdown.get();
    }
    catch (std::exception const& exception)
    {
        LOG(err, "Client shutdown failed: {}", exception.what());
    }

    m_work.reset();
    m_worker.join();
}

void NetworkComponent::connect(boost::asio::ip::tcp::endpoint endpoint)
{
    boost::asio::co_spawn(
        m_io,
        m_client->connect(endpoint.address(), endpoint.port()),
        [this](std::exception_ptr error) {
            if (error)
            {
                auto message = exceptionMessage(error);
                LOG(err, "Client connect failed: {}", message);
                publish(NetworkError{NetworkOperation::Connect, std::move(message)});
            }
        });
}

void NetworkComponent::disconnect()
{
    boost::asio::co_spawn(
        m_io,
        m_client->disconnect(),
        [this](std::exception_ptr error) {
            if (error)
            {
                auto message = exceptionMessage(error);
                LOG(err, "Client disconnect failed: {}", message);
                publish(NetworkError{NetworkOperation::Disconnect, std::move(message)});
            }
        });
}

// TODO: Templated methods for universal sendRequest
void NetworkComponent::sendRequest(s2d::protocol::PingRequest request)
{
    boost::asio::co_spawn(
        m_io,
        m_client->sendRequest(std::move(request)),
        [this](std::exception_ptr error, s2d::protocol::PongResponse response) {
            if (error)
            {
                auto message = exceptionMessage(error);
                LOG(err, "Client ping failed: {}", message);
                publish(NetworkError{NetworkOperation::Ping, std::move(message)});
                return;
            }
            publish(std::move(response));
        });
}

void NetworkComponent::sendRequest(s2d::protocol::StateSnapshotRequest request)
{
    boost::asio::co_spawn(
        m_io,
        m_client->sendRequest(std::move(request)),
        [this](
            std::exception_ptr error,
            s2d::protocol::StateSnapshotResponse response) {
            if (error)
            {
                auto message = exceptionMessage(error);
                LOG(err, "Client state snapshot failed: {}", message);
                publish(NetworkError{NetworkOperation::StateSnapshot, std::move(message)});
                return;
            }
            publish(std::move(response));
        });
}

network::ConnectionState NetworkComponent::state() const noexcept
{
    return m_client->state();
}

std::deque<NetworkResult> NetworkComponent::drainResults()
{
    std::deque<NetworkResult> results;
    {
        std::scoped_lock lock{m_resultsMutex};
        results.swap(m_results);
    }
    return results;
}

void NetworkComponent::publish(NetworkResult result)
{
    std::scoped_lock lock{m_resultsMutex};
    m_results.push_back(std::move(result));
}
} // namespace s2d::client
