Feature: In cases of singular attention, an application may fill the physical display. This is the best use of screen-space and so is the default behavior. Sometimes though the application may have a specific request, and we should try our best to fill it in that way which will most serve the user.

Scenario Outline: Application is opened in consuming mode
  Given the display-size is <display_size>
  When "<an_application>" is opened in consuming mode
  Then "<an_application>" will have size <display_size>

  Examples:
    | display_size | an_application |
    | 2048x2048    | Notes          |

Scenario Outline: Application requests a reasonable size
  Given the display-size is <display_size>
  When "<an_application>" is opened with size <reasonable_requested_size>
  Then "<an_application>" will have size <reasonable_requested_size>
  
  Examples:
    | display_size | reasonable_requested_size | an_application |
    | 2048x2048    | 1024x1024                 | Notes          |

Scenario Outline: Application requests an unreasonable size and is clipped to the screen dimensions
  Given the display-size is <display_size>
  When "<an_application>" is opened with size <unreasonable_requested_size>
  Then "<an_application>" will have size <clipped_size>
  
  Examples:
    | display_size | unreasonable_requested_size | clipped_size | an_application |
    | 2048x2048    | 2500x2500                   | 2048x2048    | Notes          |
    | 2048x2048    | 2100x2047                   | 2048x2047    | Notes          |
    | 2048x2048    | 2040x2049                   | 2040x2048    | Notes          |


