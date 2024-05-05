# `XMakeBuildSystem`

## Big Picture: `BuildSystem`

This is a sequence diagram of how `ProjectExplorer::BuildSystem` interacts with
its implementations:

```mermaid
sequenceDiagram
    User ->> BuildSystemImpl: provide data and ask for parse (impl. defined!)
    BuildSystemImpl ->> BuildSystem: call requestParse() or requestDelayedParse()
    activate BuildSystem
    BuildSystem ->> BuildSys    tem: m_delayedParsingTimer sends timeout()
    BuildSystem ->> BuildSystemImpl: call triggerParsing()
    deactivate BuildSystem
    activate BuildSystemImpl
    BuildSystemImpl ->> BuildSystem: call guardParsingRun()
    activate BuildSystem
    BuildSystem ->> ParseGuard: Construct
    activate ParseGuard
    ParseGuard ->> BuildSystem: call emitParsingStarted
    BuildSystem ->> User: signal parsingStarted()
    BuildSystem ->> BuildSystemImpl: Hand over ParseGuard
    deactivate BuildSystem
    BuildSystemImpl ->> BuildSystemImpl: Do parsing
    opt Report Success
    BuildSystemImpl ->> ParseGuard: markAsSuccess()
    end
    BuildSystemImpl ->> ParseGuard: Destruct
    ParseGuard ->> BuildSystem: emitParsingFinished()
    activate BuildSystem
    BuildSystem ->> User: Signal ParsingFinished(...)
    deactivate BuildSystem
    deactivate ParseGuard
    deactivate BuildSystemImpl
```

## The Details of `XMakeBuildSystem`

### States Overview

```mermaid
graph TD
    parse --> TreeScanner::asyncScanForFiles

    parse --> FileApiReader::parse
    FileApiReader::parse --> handleParsingSucceeded
    handleParsingSucceeded --> combineScanAndParse
    FileApiReader::parse --> handleParsingFailed
    handleParsingFailed --> combineScanAndParse

    TreeScanner::asyncScanForFiles --> handleTreeScanningFinished
    handleTreeScanningFinished --> combineScanAndParse
```

### Full Sequence Diagram

```mermaid
sequenceDiagram
    participant User
    participant ParseGuard
    participant XMakeBuildSystem
    participant FileApiReader

    alt Trigger Parsing
    User ->> XMakeBuildSystem: Any of the Actions defined for XMakeBuildSystem
    else
    User ->> XMakeBuildSystem: Signal from outside the XMakeBuildSystem
    end
    activate XMakeBuildSystem
    XMakeBuildSystem ->> XMakeBuildSystem: call setParametersAndRequestReparse()
    XMakeBuildSystem ->> XMakeBuildSystem: Validate parameters
    XMakeBuildSystem ->> FileApiReader: Construct
    activate FileApiReader
    XMakeBuildSystem ->> FileApiReader: call setParameters
    XMakeBuildSystem ->> XMakeBuildSystem: call request*Reparse()
    deactivate XMakeBuildSystem

    XMakeBuildSystem ->> XMakeBuildSystem: m_delayedParsingTimer sends timeout() triggering triggerParsing()

    activate XMakeBuildSystem

    XMakeBuildSystem ->>+ XMakeBuildSystem: call guardParsingRun()
    XMakeBuildSystem ->> ParseGuard: Construct
    activate ParseGuard
    ParseGuard ->> XMakeBuildSystem: call emitParsingStarted
    XMakeBuildSystem ->> User: signal parsingStarted()
    XMakeBuildSystem ->>- XMakeBuildSystem: Hand over ParseGuard

    XMakeBuildSystem ->>+ TreeScanner: call asyncScanForFiles()

    XMakeBuildSystem ->>+ FileApiReader: call parse(...)
    FileApiReader ->> FileApiReader: startState()
    deactivate XMakeBuildSystem

    opt Parse
    FileApiReader ->> FileApiReader: call startXMakeState(...)
    FileApiReader ->> FileApiReader: call xmakeFinishedState(...)
    end

    FileApiReader ->> FileApiReader: call endState(...)

    alt Return Result from FileApiReader
    FileApiReader ->> XMakeBuildSystem: signal dataAvailable() and trigger handleParsingSucceeded()
    XMakeBuildSystem ->> FileApiReader: call takeBuildTargets()
    XMakeBuildSystem ->>  FileApiReader: call takeParsedConfiguration(....)
    else
    FileApiReader ->> XMakeBuildSystem: signal errorOccurred(...) and trigger handelParsingFailed(...)
    XMakeBuildSystem ->> FileApiReader: call takeParsedConfiguration(....)
    end

    deactivate FileApiReader
    Note right of XMakeBuildSystem: TreeScanner is still missing here
    XMakeBuildSystem ->> XMakeBuildSystem: call combineScanAndParse()

    TreeScanner ->> XMakeBuildSystem: signal finished() triggering handleTreeScanningFinished()
    XMakeBuildSystem ->> TreeScanner: call release() to get files
    deactivate TreeScanner
    Note right of XMakeBuildSystem: All results are in now...
    XMakeBuildSystem ->> XMakeBuildSystem: call combineScanAndParse()

    activate XMakeBuildSystem
    opt: Parsing was a success
    XMakeBuildSystem ->> XMakeBuildSystem: call updateProjectData()
    XMakeBuildSystem ->> FileApiReader: call projectFilesToWatch()
    XMakeBuildSystem ->> FileApiReader: call createRawProjectParts(...)
    XMakeBuildSystem ->> FileApiReader: call resetData()
    XMakeBuildSystem ->> ParseGuard: call markAsSuccess()
    end

    XMakeBuildSystem ->> ParseGuard: Destruct
    deactivate ParseGuard

    XMakeBuildSystem ->> XMakeBuildSystem: call emitBuildSystemUpdated()
    deactivate FileApiReader
    deactivate XMakeBuildSystem
```

# `FileApiReader`

States in the `FileApiReader`.

```mermaid
graph TD
    startState --> startXMakeState
    startState --> endState
    startXMakeState --> xmakeFinishedState
    xmakeFinishedState --> endState
```

