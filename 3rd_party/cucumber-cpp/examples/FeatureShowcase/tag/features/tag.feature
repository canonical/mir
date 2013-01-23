# language: en
Feature: Tags
  In order to explain how to use tags
  I have to make this silly example

  Scenario: No tag
    Given I'm running a step from a scenario not tagged
    When I'm running a step from a scenario not tagged
    Then I'm running a step from a scenario not tagged

  @foo
  Scenario: Foo
    Given I'm running a step from a scenario tagged with @foo
    When I'm running a step from a scenario tagged with @foo
    Then I'm running a step from a scenario tagged with @foo

  @bar
  Scenario: Bar
    Given I'm running a step from a scenario tagged with @bar
    When I'm running a step from a scenario tagged with @bar
    Then I'm running a step from a scenario tagged with @bar

  @baz
  Scenario: Baz
    Given I'm running a step from a scenario tagged with @baz
    When I'm running a step from a scenario tagged with @baz
    Then I'm running a step from a scenario tagged with @baz

  @bar @foo
  Scenario: Bar and Foo
    Given I'm running a step from a scenario tagged with @bar,@foo
    When I'm running a step from a scenario tagged with @bar,@foo
    Then I'm running a step from a scenario tagged with @bar,@foo

  @pickle
  Scenario: Pickle
    Given I'm running a step from a scenario tagged with @pickle
    When I'm running a step from a scenario tagged with @pickle
    Then I'm running a step from a scenario tagged with @pickle
