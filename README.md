# Secure bootlaoder baremetal
## 1. Intrduction

``` mermaid

flowchart TD

    A[reset handler] --> B[boot mode]

    %% LEFT SIDE
    B --> C{receive update signal?}
    C -->|yes| D[update new firmware]
    D --> E[enter new firmware]
    C -->|none / updated| B

    %% MIDDLE + RIGHT
    B --> F{receive update signal?}
    F -->|Yes| G[receive new firmware<br/>store at backup bank]

    G --> H[read metadata<br/>signature + sha256]
    H --> I[verify signature in staging bank]

    I -->|NO| F
    I -->|YES| J[recalculate sha256<br/>for received firmware]

    J --> K{compare 2 hash256}
    K -->|No| F
    K -->|Yes| L[swap bank]

    L --> M[enter fw]

    %% MAIN BANK FLOW
    B --> N[verify signature main bank]
    N -->|No| F

    N -->|Yes| O{is update fw?}
    O -->|No| P{compare OK}
    P -->|yes| L
    P -->|no| F

    O -->|Yes| F


