#include <miroil/mir_prompt_session.h>

miroil::MirPromptSession::MirPromptSession(::MirPromptSession * prompt_session)
{
    this->prompt_session = prompt_session;
}

miroil::MirPromptSession::MirPromptSession(MirPromptSession const& src) = default;
miroil::MirPromptSession::MirPromptSession(MirPromptSession && src) = default;
miroil::MirPromptSession::~MirPromptSession() = default;

auto miroil::MirPromptSession::operator=(MirPromptSession const& src) -> MirPromptSession&
{
    prompt_session = src.prompt_session;
    return *this;
}

auto miroil::MirPromptSession::operator=(MirPromptSession&& src) -> MirPromptSession&
{
    prompt_session = src.prompt_session;
    return *this;
}

bool miroil::MirPromptSession::operator==(MirPromptSession const& other)
{
    return prompt_session == other.prompt_session;
}

bool miroil::MirPromptSession::new_fds_for_prompt_providers(unsigned int /*no_of_fds*/, MirClientFdCallback /*callback*/, void * /*context*/)
{
//    return mir_prompt_session_new_fds_for_prompt_providers(prompt_session, no_of_fds, callback, context);
    return false;
}
