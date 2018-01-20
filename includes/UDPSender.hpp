#ifndef UDP_SENDER_HPP
#define UDP_SENDER_HPP

#include <arpa/inet.h>
#include <cstring>
#include <deque>
#include <experimental/optional>
#include <cmath>
#include <mutex>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

namespace Statsd
{
/*!
 *
 * UDP sender
 *
 * A simple UDP sender handling batching.
 * Its configuration can be changed at runtime for
 * more flexibility.
 *
 */
    class UDPSender final
    {
    public:

        //!@name Constructor and destructor
        //!@{

        //! Constructor
        UDPSender(
                const std::string& host,
                const uint16_t port,
                const std::experimental::optional<uint64_t> batchsize = std::experimental::nullopt) noexcept;

        //! Destructor
        ~UDPSender();

        //!@}

        //!@name Methods
        //!@{

        //! Sets a configuration { host, port }
        void setConfig(const std::string& host, const uint16_t port) noexcept;

        //! Send a message
        void send(const std::string& message) noexcept;

        //! Returns the error message as an optional string
        std::experimental::optional<std::string> errorMessage() const noexcept;

        //!@}

    private:

        // @name Private methods
        // @{

        //! Initialize the sender and returns true when it is initialized
        bool initialize() noexcept;

        //! Send a message to the daemon
        void sendToDaemon(const std::string& message) noexcept;

        //!@}

    private:

        // @name State variables
        // @{

        //! Is the sender initialized?
        bool m_isInitialized{ false };

        //! Shall we exit?
        bool m_mustExit{ false };

        //!@}

        // @name Network info
        // @{

        //! The hostname
        std::string m_host;

        //! The port
        uint16_t m_port;

        //! The structure holding the server
        struct sockaddr_in m_server;

        //! The socket to be used
        int m_socket{ -1 };

        //!@}

        // @name Batching info
        // @{

        //! Shall the sender use batching?
        bool m_batching{ false };

        //! The batching size
        uint64_t m_batchsize;

        //! The queue batching the messages
        mutable std::deque<std::string> m_batchingMessageQueue;

        //! The mutex used for batching
        std::mutex m_batchingMutex;

        //! The thread dedicated to the batching
        std::thread m_batchingThread;

        //!@}

        //! Error message (optional string)
        std::experimental::optional<std::string> m_errorMessage;
    };

}

#endif