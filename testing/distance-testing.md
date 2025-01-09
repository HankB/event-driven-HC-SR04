# Distance Testing

## Purpose

Test the code and sensor over the published range to determine reliability, precision and accuracy. Some documentation I have found indicates a range of 2-400 centimeters (0.78-157.5 inches.)

With the existing Python code that performs this function, I have coded it to take 5 readings, throw out the highest and lowest and then average the rest to get usable results.

## Preliminary testing

Place the sensor about 2 1/2 feet above the ground aimed at a garage door 224 inches away. Readings were all over the place and not reflective of the actual distance. (This is well past the qouted range of the sensor.) Repeating the test at 9 feet (108 inches) resulted in 50 readings that looked to be (at a glance) within half an inch of 108 inches.

## Plan

Situate the sensor such that distance to a flat surface can be controlled and take readings at various distances within the quoted range. Use the reading at 2 cm to establish the point on the sensor from which distance is measured.

Import the readings into a spreadsheet to facilitate further analysis.

### Establish datum

Determine where the 2 cm distance is measured from by adjusting the position of the sensor from a flat surace until it reports 2 cm (0.78 inch) distance. This turns out to be about 5/8 inch from the front edge of ths sensor. The 50 readings average to 0.776 inch.
