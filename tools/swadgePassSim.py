from enum import Enum
import random
from random import randint
from multiprocessing import Pool
from datetime import datetime


class swadgeState(Enum):
    SLEEPING = 1
    WAKING = 2
    TRANSMITTING = 3
    LISTENING = 4
    ENTERING_SLEEP = 5


class swadgeSim:
    def __init__(self, listenMs: int, minSleepMs: int, maxSleepMs: int):
        self.state: swadgeState = swadgeState.SLEEPING
        self.clockMs = 0
        self.transitionTimer = 1
        self.listenMs = listenMs
        self.minSleepMs = minSleepMs
        self.maxSleepMs = maxSleepMs

    def simTimeStep(self):
        # Step the ms clock
        self.clockMs += 1

        # Check for transitions
        self.transitionTimer -= 1
        if 0 == self.transitionTimer:
            match self.state:
                case swadgeState.SLEEPING:
                    # Go from sleeping to waking for 4ms
                    self.state = swadgeState.WAKING
                    self.transitionTimer = 4
                    pass
                case swadgeState.WAKING:
                    # Go from waking to transmitting for 1ms
                    self.state = swadgeState.TRANSMITTING
                    self.transitionTimer = 1
                    pass
                case swadgeState.TRANSMITTING:
                    # Go from transmitting to listening for self.listenMs
                    self.state = swadgeState.LISTENING
                    self.transitionTimer = self.listenMs
                    pass
                case swadgeState.LISTENING:
                    # Go from listening to entering sleep for 1ms
                    self.state = swadgeState.ENTERING_SLEEP
                    self.transitionTimer = 1
                    pass
                case swadgeState.ENTERING_SLEEP:
                    # Go from transmitting to listening for random time
                    self.state = swadgeState.SLEEPING
                    self.transitionTimer = randint(
                        self.minSleepMs, self.maxSleepMs)
                    pass

    def getState(self) -> swadgeState:
        return self.state

    def goToSleep(self):
        self.transitionTimer = 1
        self.state = swadgeState.LISTENING


def runTest(numSwadges: int, listenMs: int, minSleepMs: int, maxSleepMs: int, msTest: int) -> int:

    # Create virtual swadges
    swadges: list[swadgeSim] = []
    for _ in range(numSwadges):
        swadges.append(swadgeSim(listenMs, minSleepMs, maxSleepMs))

    # Keep track of passes
    passCount = 0

    # For every millisecond in a day
    for _ in range(msTest):

        # For each swadge, simulate this ms
        for swadge in swadges:
            swadge.simTimeStep()

        # For each combination of swadges
        for srcIdx in range(len(swadges)):
            for dstIdx in range(srcIdx + 1, len(swadges)):
                # check if they hear each other
                if ((swadgeState.TRANSMITTING == swadges[srcIdx].getState()) and (swadgeState.LISTENING == swadges[dstIdx].getState())) or \
                        ((swadgeState.TRANSMITTING == swadges[dstIdx].getState()) and (swadgeState.LISTENING == swadges[srcIdx].getState())):
                    # One transmits when the other listens, count it!
                    passCount += 1

    return [numSwadges, listenMs, minSleepMs, maxSleepMs, msTest, passCount]

if __name__ == '__main__':

    random.seed(datetime.now().timestamp())

    timeArgs = []
    numSwadges = 2
    hours = 24
    testMs = (hours * 60 * 60 * 1000)
    for listenTime in range(500, 1500 + 1, 100):
        for sleepTime in range(5000, 15000 + 1, 1000):
            quarterSleep = int(sleepTime/4)
            minSleep = sleepTime - quarterSleep
            maxSleep = sleepTime + quarterSleep
            timeArgs.append((2, listenTime, minSleep, maxSleep, testMs))

    print(','.join(['Swadges', 'listenMs', 'minSleepMs', 'maxSleepMs', 'msTest', 'passCount', 'passesPerHour']))

    with Pool(28) as p:
        results = p.starmap(runTest, timeArgs)
        for result in results:
            passCount = result[5]
            timeMs = result[4]
            passPerHour = (passCount * (1000 * 60 * 60)) / (timeMs)
            result.append(passPerHour)

            print(','.join([str(x) for x in result]))
