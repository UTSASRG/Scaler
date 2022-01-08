def countSamplesbyTID():
    # Store number of samples recorded for each thread key:value = tid:samples recorded
    threadSamples = {}
    # Pass in collapsed stack from flamegraph stack collapser output
    fold = open("flameTest2.folded")
    currentTid = ''
    sampleTotal = 0
    # Iterate through the lines of the folded file
    for line in fold:
        # We know that the lines are separated by semicolons ;
        lineList = line.split(';')
        # Then we want to grab the sample count for this event and we know that the last element contains it and is seperated by white space
        lastEle = lineList[-1].split()
        # TID is a part of the command section and I know that the TID is past the / character, so I split using that
        comm = lineList[0].split('/')
        # comm[1] indicates the TID and if our currentTID is not equal the the TID we just grabbed, then we need to update the dictionary
        if currentTid != comm[1]:
            # Base case of when I initialized the currentTID var to '', no sample count has been accumulated yet, so I simply update currentTID
            if currentTid == '':
                currentTid = comm[1]
            # Otherwise just run normally
            else:
                threadSamples[currentTid] = sampleTotal
                currentTid = comm[1]
                sampleTotal = 0
        # After each line read, we increment our sample total by the recorded sample count from the line
        sampleTotal += int(lastEle[-1])
    # For updating the dictionary with the last thread
    threadSamples[currentTid] = sampleTotal
    fold.close()
    print(threadSamples)
    return
countSamplesbyTID()


