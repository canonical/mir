Feature: In order to best use screen space an application may fill the screen

Scenario Outline: Application is open in consuming mode
  Given the display-size is <display_size>
  When Firefox is opened in consuming mode
  Then Firefox will have size <display_size>

  Examples:
    | display_size |
    | 2048x2048    | 

Scenario Outline: Application requests a reasonable size
  Given the display-size is <display_size>
  When Firefox is opened with size <reasonable_requested_size>
  Then Firefox will have size <reasonable_requested_size>
  
  Examples:
    | display_size | reasonable_requested_size |
    | 2048x2048    | 1024x1024                 |