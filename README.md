# Embedded-Systems
#  Light Race

Sadie Park and Rhian Seneviratne

More info on : <https://docs.google.com/document/d/1H5CtbLAaPCClw6-tk6SlDpHkeavHgzVcR60UUApjrRc/edit?usp=sharing>

***

## Introduction

We are going to create a game meant for two players using an embedded system with the MSP432P111 controlling the main functions of the game. The idea behind the game is a race between two players to click a button that corresponds to whichever LED lights up on their side. There will be a speaker that plays a note whenever an LED lights up, and its volume will be controlled by a potentiometer. Each player has 3 possible LEDS that light up and 3 possible buttons to press. The first player to hit their button will earn a point, which will add to their score that is displayed on the connected LCD. Each player has their own timer and an interrupt is triggered that will store the time elapsed to determine who pressed first. The first player to 10 points will win the game. A start and restart button is added to reset the game score and then begin the game. 

This project utilizes many of the subjects we learned in class. Communicating with the LCD to display characters, using buttons to trigger interrupts, the use of timers, and outputting notes onto a piezo buzzer.

## Objectives and specifications

- [ ] Two users must click their button corresponding to their light as fast as possible
- [ ] When the button is pressed an interrupt can be triggered for time elapsed
- [ ] Speaker produce certain note when the LED light is on
- [ ] First to 10 points wins game
- [ ] LCD Panel acts as scoreboard that stores the score of user1 on top row and user2 on bottom
- [ ] Start/Restart Button

## Conceptual design description

- User Controls  
  Each user has three buttons to score and one needs to be picked based off the LED that lights up  
  A start/reset button that is pressed once at the end of the game to reset, then pressed again to start a new one.
- LCD display  
  Displays the score of each user (“Player 1 : yy”) on top row and (“Player 2: xx”) on bottom row  
  When the winner is determined, it displays the name of user (“Player X win !”)
- LED Circuit  
  A LED is selected based off a random integer, and lights up while playing its distinct note on the piezo buzzer  
  Speed between rounds can be changed by a potentiometer input from ADC conversion
- Passive buzzer  
  Plays distinct note for different LED light  
  Plays winning song when user wins  
  Potentiometer to control volume of buzzer

## Tasks and priorities

- Design a test plan(Assigning Timers, ports, interrupts, etc. )
- Develop software
- Configure I/O connections for buttons and LEDs
- ADC Conversion for potentiometer controlling speed of game
- Configure LCD class, header and main integration for outputting user score variables
- Buzzer class and header for outputting notes final song
- Light Race main class
- Build hardware
- Test and debug the hardware
