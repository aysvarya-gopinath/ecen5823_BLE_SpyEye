Note: For all assignments and Energy Profiler measurements you’ll be taking this semester,  Peak measurements are instantaneous measurements taken at a specific point in time. In the Energy Profiler, this is accomplished by left-clicking at a location along the time axis.
Average measurements are measurements that are taken over a time-span. In the Energy Profiler, this is accomplished by left-clicking and dragging a region along the time axis.

Please include your answers to the questions below with your submission, entering into the space below each question
See [Mastering Markdown](https://guides.github.com/features/mastering-markdown/) for github markdown formatting if desired.

**1. How much current does the system draw (instantaneous measurement) when a single LED is on with the GPIO pin set to StrongAlternateStrong?**
   Answer:
         The instantaneous current with the gpio pin set to StringAlternateStrong for LED0 was 5.5mA and LED1 was 5.58mA.

**2. How much current does the system draw (instantaneous measurement) when a single LED is on with the GPIO pin set to WeakAlternateWeak?**
   Answer:
         The instantaneous current with the gpio pin set to WeakAlternateWeak for LED0 was 5.40mA and LED1 was 5.54mA.


**3. Is there a meaningful difference in current between the answers for question 1 and 2? Please explain your answer, referencing the main board schematic, WSTK-Main-BRD4001A-A01-schematic.pdf or WSTK-Main-BRD4002A-A06-schematic.pdf, and AEM Accuracy in the ug279-brd4104a-user-guide.pdf. Both of these PDF files are available in the ECEN 5823 Student Public Folder in Google drive at: https://drive.google.com/drive/folders/1ACI8sUKakgpOLzwsGZkns3CQtc7r35bB?usp=sharing . Extra credit is available for this question and depends on your answer.**
   Answer:There is a negligible difference in the currents with the different drive strengths.The static current of the LED is found to be 0.5mA. With strong drive strength, the GPIO supplies 10mA and 1mA for the weak drive strength to the load.
   Therefore, the minor variations in the instantaneous current between the two drive strength settings are primarily due to the fact that the drive strength changes have minimal impact on the current drawn by the LED.
   When there is lower drive strength, there is lesser slew rate and as drive strength increases, the slew rate increases. 
      The drive strength influences the slew rate but doesnt have a significant  effect on the instataneous current drawn by the LEDand thus, does not produce a noticeable change in the current.
    For curents above 0.25mA the AEM is accurate within 0.1mA and can detect current changes as fine as 100nA.Hence the AEM is capable of fine-grained measurements and the above results are not affected by its limitations.
  

**4. With the WeakAlternateWeak drive strength setting, what is the average current for 1 complete on-off cycle for 1 LED with an on-off duty cycle of 50% (approximately 1 sec on, 1 sec off)?**
   Answer:
         The average current with WeakAlternateWeak drive strength when one LED was on is 5.27mA for LED0 with a 50% duty cycle.



**5. With the WeakAlternateWeak drive strength setting, what is the average current for 1 complete on-off cycle for 2 LEDs (both on at the time same and both off at the same time) with an on-off duty cycle of 50% (approximately 1 sec on, 1 sec off)?**
   Answer:
         The average current with WeakAlternateWeak drive strength when both LED0 and LED1 were on is 5.51mA  with a 50% duty cycle.

