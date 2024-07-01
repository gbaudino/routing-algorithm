import pandas as pd
from simu import *

def sanitize(simulationName, caseName):

    rawPath = resultCaseDir(simulationName, caseName)
    sanitized_path = rawPath + 'processed'

    # Read the data from csv in data path
    filepath = f"{rawPath}{sep}raw.csv"
    df = pd.read_csv(filepath)

    # Drop the columns that are not needed
    df = df.drop(columns= ['run','type'])

    # Save document as processed.csv
    df.to_csv(f"{resultCaseDir(simulationName, caseName)}{sep}processed.csv", index=False)