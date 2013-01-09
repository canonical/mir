Feature: In order to best use screen space an application may fill the screen. This is the best use of space and so is the default behavior. Sometimes the application may know best though so we should let the application request a size as well.

Scenario Outline: Application is open in consuming mode
  Given the display-size is <display_size>
  When "Firefox" is opened in consuming mode
  Then "Firefox" will have size <display_size>

  Examples:
    | display_size |
    | 2048x2048    | 

Scenario Outline: Application requests a reasonable size
  Given the display-size is <display_size>
  When "Firefox" is opened with size <reasonable_requested_size>
  Then "Firefox" will have size <reasonable_requested_size>
  
  Examples:
    | display_size | reasonable_requested_size |
    | 2048x2048    | 1024x1024                 |

Scenario Outline: Application requests an unreasonable size and is clipped to the screen dimensions
  Given the display-size is <display_size>
  When "Firefox" is opened with size <unreasonable_requested_size>
  Then "Firefox" will have size <clipped_size>
  
  Examples:
    | display_size | unreasonable_requested_size | clipped_size |
    | 2048x2048    | 2500x2500                   | 2048x2048    |
    | 2048x2048    | 2100x2047                   | 2048x2047    |
    | 2048x2048    | 2040x2049                   | 2040x2048    |


