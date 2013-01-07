Feature: In order to best use screen space an application may fill the screen

Scenario Outline: Application is open in consuming mode
  Given The display-size is <display_size>
  When Firefox is opened in consuming mode
  Then Firefox will have size <display_size>

  Examples:
    | display_size |
    | 2048x2048    | 
