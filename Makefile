build:
	@cd src && make
	@cd ..

case1: build
	@python3 simu.py case1

case2: build
	@python3 simu.py case2

run: build
	@python3 simu.py testCase

clean:
	@cd src && make clean
	@cd ..