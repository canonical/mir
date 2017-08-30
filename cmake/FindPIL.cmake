execute_process(
  COMMAND python3 -c "from PIL import Image"
  RESULT_VARIABLE HAVE_PIL
)

if (NOT ${HAVE_PIL} EQUAL 0)
  message(FATAL_ERROR "Python Imaging Library (PIL) not found")
endif()
