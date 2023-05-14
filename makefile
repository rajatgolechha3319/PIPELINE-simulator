compile: sample sample2 sample3 sample4

sample: 5stage.cpp 5stage.hpp
	g++ 5stage.cpp 5stage.hpp -o 5stage

sample2: 5stage_bypass.cpp 5stagebypass.hpp
	g++ 5stage_bypass.cpp 5stagebypass.hpp -o 5stagebypass

sample3: 79stage.cpp 79stage.hpp
	g++ 79stage.cpp 79stage.hpp -o 79stage

sample4: 79stage_bypass.cpp 79stagebypass.hpp
	g++ 79stage_bypass.cpp 79stagebypass.hpp -o 79stagebypass

sample5: test.cpp
	g++ test.cpp -o test

run_5stage: 
	./5stage input.asm

run_5stage_bypass:
	./5stagebypass input.asm

run_79stage:
	./79stage input.asm

run_79stage_bypass:
	./79stagebypass input.asm

run_branch:
	./test 

clean:
	rm 5stage 5stagebypass 79stagebypass 79stage test



