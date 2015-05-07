struct NullInputTargeter : public msh::InputTargeter
{
    NullInputTargeter() = default;
    virtual ~NullInputTargeter() noexcept(true) = default;

    void set_focus(std::shared_ptr<mi::Surface> const&) override
    {
    }

    void clear_focus() override
    {
    }
};
