//
// Created by SherSpidey on 2026/5/26.
//

#ifndef NEXENGINE_COMMAND_H
#define NEXENGINE_COMMAND_H

namespace Nex
{
    enum class CommandType : uint32_t
    {
        Invalid = 0,

        // command type begins here

        // scene operation
        Add_Object,
        Remove_Object,
    };

    struct CommandHeader
    {
        CommandType type;

        uint32_t request_id;
    };

    struct Command
    {
        CommandHeader header;

        // commands begin here
    };

    enum class ResponseType : uint32_t
    {
        Invalid = 0,

        // command type begins here
    };

    struct ResponseHeader
    {
        ResponseType type;

        uint32_t request_id;
    };


    struct Response
    {
        ResponseHeader header;

        // responses begin here
    };

    using ResponseCallback = void(*)(const Response&, void*);
}

#endif //NEXENGINE_COMMAND_H
