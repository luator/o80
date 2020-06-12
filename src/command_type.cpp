#include "o80/internal/command_type.hpp"

namespace o80
{
CommandType::CommandType(Speed _speed) : type(Type::SPEED), speed(_speed.value)
{
}

CommandType::CommandType(Iteration _iteration)
    : type(Type::ITERATION),
      iteration(_iteration.value, _iteration.relative, _iteration.do_reset)
{
}

CommandType::CommandType(Duration _duration)
    : type(Type::DURATION), duration(_duration.value)
{
}

CommandType::CommandType() : type(Type::DIRECT)
{
}

}
