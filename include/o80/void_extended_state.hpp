// Copyright (c) 2019 Max Planck Gesellschaft
// Author : Vincent Berenz

#pragma once

namespace o80
{
  /* ! EXTENDED_STATE is a template parameter of BackEnd, FrontEnd and Standalone
   *   that the user code may use to enrich Observation with arbitrary data.
   *   In case such arbitrary data is not required, VoidExtendedState can be used
   *   as template paramter.
   */
class VoidExtendedState
{
public:
    VoidExtendedState()
    {
    }
    VoidExtendedState(const VoidExtendedState& other)
    {
    }
    VoidExtendedState(VoidExtendedState&& other) noexcept
    {
    }
    VoidExtendedState& operator=(const VoidExtendedState& other)
    {
        return *this;
    }
    VoidExtendedState& operator=(VoidExtendedState&& other) noexcept
    {
        return *this;
    }
    void console() const
    {
        std::cout << "empty extended state" << std::endl;
    }
    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(foo);
    }
    char foo;
};
}
