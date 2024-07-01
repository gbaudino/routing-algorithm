import os
import sys
import sanity

# config variables
sep = os.sep
codePath = "src"
dataPath = "data"
plotsPath = "plots"
execPath = f"{codePath}{sep}exec"
defaultIniPath = f"{codePath}{sep}default.ini"
iniPath = f"{codePath}{sep}omnetpp.ini"
generationIntervals = [round(0.1 + x * 0.01, ndigits = 2) for x in range(0, 91)]

# MACROS
def resultDir(simulationName):               return f"{dataPath}{sep}{simulationName}"
def resultCaseDir(simulationName, caseName): return f"{resultDir(simulationName)}{sep}{caseName}"
def plotsDir(simulationName):                return f"{plotsPath}{sep}{simulationName}"
def plotCasePath(simulationName, caseName):  return f"{plotsDir(simulationName)}{sep}{caseName}"

import graphs

def simulate(simulationName):
    assert simulationName != "", "Simulation name cannot be empty"
    print(f"Simulating {simulationName}...")

    if not os.path.exists(execPath):
        # Compile executable if not exists
        os.system(f"cd {codePath} && make clean && make && cd ..")

    if os.path.exists(resultDir(simulationName)):
        # Delete directory if exists
        os.system(f"rm -rf {resultDir(simulationName)}")
    
    # Create directory for results
    os.system(f"mkdir {resultDir(simulationName)}")

    # check needed files
    assert os.path.exists(execPath), f"Executable not found at {execPath}"
    assert os.path.exists(defaultIniPath), f"default ini file not found at {defaultIniPath}"
    
    # run simulation
    for gi in generationIntervals:
        print(f"Running simulation for generation interval {gi}...")

        # copy default ini as omnetpp.ini and add generation interval
        os.system(f"cp {defaultIniPath} {iniPath}")
        if "part2" in simulationName:
            os.system(f"echo '\nNetwork.node[{{0,1,2,3,4,6,7,8}}].app.interArrivalTime = exponential({gi})' >> {iniPath}")
            os.system(f"echo '\nNetwork.node[{{0,1,2,3,4,6,7,8}}].app.packetByteSize = 125000' >> {iniPath}")
            os.system(f"echo '\nNetwork.node[{{0,1,2,3,4,6,7,8}}].app.destination = 5' >> {iniPath}")
        else:
            os.system(f"echo '\nNetwork.node[{{0,2}}].app.interArrivalTime = exponential({gi})' >> {iniPath}")
            os.system(f"echo '\nNetwork.node[{{0,2}}].app.packetByteSize = 125000' >> {iniPath}")
            os.system(f"echo '\nNetwork.node[{{0,2}}].app.destination = 5' >> {iniPath}")

        retCod = os.system(f".{os.sep}{execPath} -f {iniPath} -n {codePath} -u Cmdenv")
        assert retCod == 0, "Simulation failed"
        
        specificPath = f"{resultCaseDir(simulationName, gi)}"
        # move results of simulation to results directory
        os.system(f"mv {codePath}{sep}results {specificPath}")

        # delete omnetpp.ini and .cmdenv-log
        os.system(f"rm {iniPath}")
        os.system(f"rm .cmdenv-log")

        # Convert .sca to .anf file
        resultFileName = f"{specificPath}{sep}General-#0"
        os.system(f"opp_scavetool x {resultFileName}.sca {resultFileName}.vec -o {resultCaseDir(simulationName, gi)}{sep}raw.csv")
        # Delete .sca .vec and .vci files
        os.system(f"rm {resultFileName}.sca")
        os.system(f"rm {resultFileName}.vec")
        os.system(f"rm {resultFileName}.vci")

        # Sanitize data
        sanity.sanitize(simulationName, gi)

def graphic(simulationName):
    
    # Check if plots directory exists
    if os.path.exists(plotsDir(simulationName)):
        # Delete directory if exists
        os.system(f"rm -rf {plotsDir(simulationName)}")
    
    if not os.path.exists(plotsPath):
        # Create directory for plots if not exists
        os.system(f"mkdir {plotsPath}")
        
    # Create directory for plots of this simulation
    os.system(f"mkdir {plotsDir(simulationName)}")

    graphs.delayPaquetes(simulationName)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 simu.py <simulationName>")
        exit(1)
    elif (len(sys.argv) > 2):
        print("Usage: python3 simu.py <simulationName>")
        exit(1)
    simulate(sys.argv[1])
    graphic(sys.argv[1])

    # remove data directory
    os.system(f"rm -rf {dataPath}")


