

TEST(TestDisplayConfiguration, config)
{
    mp::Connection connect_result; 
    mcl::DisplayConfiguration internal_config(connect_result);

    MirDisplayInfo info, new_info;
    info = internal_config;
   
    EXPECT_THAT(info, matches(connect_result); 

    int called_count = 0u;
    auto fn = [&]()
    {
        called_count++;
    };
    internal_config.set_change_callback(fn);

    mp::DisplayConfiguration new_result; 
    new_info = internal_config.update(new_result);
    EXPECT_THAT(info.matches(new_inof);
    EXPECT_EQ(1u, called_count);
}
