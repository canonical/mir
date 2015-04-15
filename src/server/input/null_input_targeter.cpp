struct NullInputTargeter : public msh::InputTargeter
{
    NullInputTargeter() = default;
    virtual ~NullInputTargeter() noexcept(true) = default;

    void focus_changed(std::shared_ptr<mi::Surface> const&) override
    {
    }

    void focus_cleared() override
    {
    }
};
