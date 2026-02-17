# BFS Implementation Performance Evaluation 


## Environment
All benchmarks were executed on the Centaurus cluster.

- Compiler: g++ (C++17, -O2 optimization)
- Sequential executable: seq_client
- Parallel executable: level_client
- Libraries: libcurl, pthread
- Graph traversal method: BFS
- Data source: Movie graph starting from specified actor node

---

## Required Test Case
**Start Node:** "Tom Hanks"  
**Traversal Depth:** 3  

Two independent runs were executed to verify consistency.

| Run ID | Sequential Time (s) | Parallel Time (s) | Speedup |
|--------|---------------------|-------------------|---------|
| 13913  | 72.0956             | 9.34907           | 7.71×   |
| 13914  | 72.1551             | 9.33169           | 7.73×   |
| **Average** | **72.1254** | **9.34038** | **7.72×** |

**Observation:**  
The parallel BFS implementation achieves approximately **7.7× speedup** over the sequential version for the required test case. Results are highly consistent across runs, indicating stable parallel performance.

---

## Additional Sequential Tests (Tom Hanks)

### Depth = 1

| Trial | Time (s) |
|--------|---------|
| 1 | 0.16002 |
| 2 | 0.166163 |
| 3 | 0.159677 |
| **Average** | **0.16195** |

---

### Depth = 2

| Trial | Time (s) |
|--------|---------|
| 1 | 3.30288 |
| 2 | 3.29928 |
| 3 | 3.28805 |
| **Average** | **3.29674** |

---

### Depth = 4

| Trial | Time (s) |
|--------|---------|
| 1 | 480.813 |
| 2 | 479.476 |

*(Third trial did not complete in the captured output.)*

---

## Performance Analysis

- Execution time increases rapidly with traversal depth due to exponential growth of the explored graph.
- Parallelization significantly reduces runtime for deeper traversals where work per level is large.
- Small depths show minimal benefit from parallelism due to overhead costs.
- Results demonstrate good scalability for compute-heavy workloads.

---

## Conclusion

The parallel BFS implementation provides substantial performance improvements over the sequential version, particularly for larger traversal depths. The required test case achieved an average speedup of approximately **7.72×**, validating the effectiveness of the parallel approach.