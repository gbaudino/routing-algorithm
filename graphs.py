import matplotlib.pyplot as plt 
import numpy as np
import pandas as pd

from simu import *


def delayPaquetes(simulationName):
    """Plot average delay to receive packets in function of generation intervals"""

    averageDelay = []
    for gi in generationIntervals:
        # Get processed data
        processedData = pd.read_csv(f"{resultCaseDir(simulationName, gi)}{sep}processed.csv")
        
        # Get delay to receive packets
        delay = (processedData[(processedData['module'] == 'Network.node[5].app') & \
                                (processedData['name'] == 'Delay')]['vecvalue'].values[0:])

        # Get time
        time = (processedData[(processedData['module'] == 'Network.node[5].app') & \
                                (processedData['name'] == 'Delay')]['vectime'].values[0:])
        
        delay = list(map(float,delay[0].split()))
        time = list(map(float,time[0].split()))
        
        # Calculate average delay
        averageDelay.append(np.mean(delay))

    # Plot average delay
    plt.plot(generationIntervals, averageDelay, 'o-')
    plt.xlabel('Generation interval (s)')
    plt.ylabel('Average delay (s)')
    plt.title('Average delay to receive packets in function of generation intervals')
    plt.savefig(f"{plotsDir(simulationName)}{sep}delayPaquetes.png")
    plt.close()
