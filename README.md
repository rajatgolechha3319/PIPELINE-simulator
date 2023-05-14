# COL 216 ASSIGNMENT 2

This is the implementation of the pipeline simulator for a 5 stage and a 7-9 stage pipeline in the c++ language.

## FILES

1. `5stage.hpp` : This file contains the implementation of the 5 stage pipeline.
2. `5stagebypass.hpp` : This file contains the implementation of the 5 stage pipeline with bypassing.
3. `79stage.hpp` : This file contains the implementation of the 7-9 stage pipeline.
4. `79stagebypass.hpp` : This file contains the implementation of the 7-9 stage pipeline with bypassing.
5. `BranchPredictor.hpp` : This file contains the struct branch predictor.
6. `5stage.cpp` : This file contains the main function for the 5 stage pipeline.
7. `5stagebypass.cpp` : This file contains the main function for the 5 stage pipeline with bypassing.
8. `79stage.cpp` : This file contains the main function for the 7-9 stage pipeline.
9. `79stagebypass.cpp` : This file contains the main function for the 7-9 stage pipeline with bypassing.
10. `input.asm` : This file contains the assembly code for the input.
11. `report.pdf` : This file contains the report for the assignment with clock cycles, comparison plots and graphs.
12. `test.cpp` : This file contains the running mechanism for the branch predictor.

## MAKEFILE
The makefile contains the following commands:
1. `compile` : This command compiles the code and generates the executable files.
2. `run_5stage` : This command runs the 5 stage pipeline.
3. `run_5stage_bypass` : This command runs the 5 stage pipeline with bypassing.
4. `run_79stage` : This command runs the 7-9 stage pipeline.
5. `run_79stage_bypass` : This command runs the 7-9 stage pipeline with bypassing.
6. `test` : This command runs the branch predictor. You need to give input from 0/1/2/3 to run. 
7. `clean` : This command removes the executable files.
To run the code, use the following commands:
```bash
make compile
make run_5stage
make run_5stage_bypass
make run_79stage
make run_79stage_bypass
make test
make clean
```