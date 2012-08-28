#include <egl_mock.h>

TEST(EglMock, demo)
{
    using namespace ::testing;

    mir::EglMock mock;
    EXPECT_CALL(mock, eglGetError()).Times(Exactly(1));
    eglGetError();
}

#define EGL_MOCK_TEST(test, egl_call, egl_mock_call)                   \
    TEST(EglMock, test_##test)                                         \
    {                                                                  \
        using namespace ::testing;                                     \
                                                                       \
    mir::EglMock mock;                                                 \
    EXPECT_CALL(mock, egl_mock_call).Times(Exactly(1));                \
    egl_call;                                                          \
    }                                                                  \

EGL_MOCK_TEST(eglGetError, eglGetError(), eglGetError());
EGL_MOCK_TEST(eglGetDisplay, eglGetDisplay(NULL), eglGetDisplay(NULL));
