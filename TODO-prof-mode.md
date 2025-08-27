## AI Generated

## 🎯 Phase 1: Core Foundations (must-have)
- [ ] **Integrate Metadata into Test Results**  
  - Ensure `Weight`, `ProfOnly`, and `Assignment` attributes are stored with each test result  

- [ ] **Grading Engine Enhancements**  
  - Implement weighted score calculation for individual tests  
  - Implement per-assignment aggregation of scores  

- [ ] **Professor vs Student Mode Separation**  
  - Student mode excludes `ProfOnly` tests from execution and results  
  - Professor mode includes `ProfOnly` tests and uses them in scoring  

---

## 📊 Phase 2: Reporting & CLI
- [ ] **Professor-Mode Reporting**  
  - Human-readable output showing assignment totals, test weights, and pass/fail status  
  - Mark `ProfOnly` tests as visible only in professor mode  

- [ ] **Assignment Selection via CLI**  
  - Add flag to run a single assignment (e.g. `--assignment=hw1`)  
  - Default remains running all assignments  

- [ ] **Machine-Readable Output**  
  - JSON or CSV export of scores and metadata  

---

## 🛠️ Phase 3: Robustness & UX
- [ ] **Metadata Handling Improvements**  
  - Provide defaults (e.g., weight = 1 if unspecified)  
  - Warn or handle duplicate assignment names gracefully  

- [ ] **Optional Metadata Configuration**  
  - Support file-level `FILE_TESTS_META` to reduce duplication  
  - Ensure consistency between file-level and test-level metadata  

---

## 🚀 Phase 4: Stretch Goals (Post-MVP)
- [ ] **Partial Credit Support**  
  - Allow grouped tests where passing some yields partial score  

- [ ] **External Assignment Config**  
  - Load assignment weights and settings from JSON/YAML instead of code  

- [ ] **Selective Reruns**  
  - CLI option to run only failed tests (`--failed`)  

- [ ] **Enhanced Output Formatting**  
  - Pretty-print tables with optional color for professor reports  
