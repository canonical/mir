# language: en
Feature: Table
  In order to explain how to use tagbles
  I have to make this silly example

  Scenario: No tag
    Given the following actors are still active:
      | name           | born |
      | Al Pacino      | 1940 |
      | Robert De Niro | 1943 |
      | George Clooney | 1961 |
      | Morgan Freeman | 1937 |
    When Morgan Freeman retires
    Then the oldest actor should be Al Pacino
