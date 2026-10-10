***Project Title and Brief Description***

**Title:** Inverted Index and Code-Based Search Engine.  

**Description:** The project is a standalone, high-performance search engine designed to quickly find keywords, functions, variables, and classes across numerous source-code files and text documents. It functions by creating an inverted index that maps words to their file paths and exact locations, enabling instant lookups instead of scanning documents sequentially.


***Problem Statement/Objective*** 

**Problem Statement:** Traditional search techniques read documents step-by-step, which is highly time-consuming for developers searching through huge code repositories or databases.

**Objective:** To develop a fast, organized search system using inverted indexing, hashing, and data structures to retrieve relevant files instantly without scanning the entire dataset.   


***Team Members*** **(Name: Code Acer (ID: DSCPP-III-2026-T112))** 

1. Anmol Gupta: Team Lead (Student ID: 2510385219).  
2. Harshit kumar: Developer (Student ID: 2510012451).
3. Atharva Goyal: Developer/Docs (Student ID: 2510360047).
4. Navneet Nainwal: DB/Testing (Student ID: 2510010627).

  
***Technologies/Tools Used***

**Programming Languages:** C, C++, HTML, CSS, JavaScript.  
**Data Structures:** Hash tables, linked lists, trees, arrays, graphs, priority queues. 
**Development Tools:** VS Code, Git, GitHub.   
**Libraries/Frameworks:** STL, DOM, C/C++ standard libraries.   
**Platforms:** Windows/Linux.   


***Project Setup/Installation Instructions***

The provided documents do not contain instructions for setting up or installing the project.


***Major Features/Modules***

**Directory Walker:** Reads source files using C++ file streams and assigns document IDs.
**Lexical Tokenizer:** Strips comments and noise from raw code to generate clean text tokens.
**Inverted Index:** Stores a vocabulary and dynamic posting lists to map keywords to specific documents and locations.
**Query Engine:** Parses user queries using a lexer, applies Boolean logic, and retrieves matching document nodes from the index.
**Min-Heap Ranker:** Uses TF-IDF scoring to evaluate document relevance and outputs ranked search results.
**Binary Index Persistence:** Saves indexed data to avoid having to rebuild the index for subsequent searches.


***Current Project Status/Progress***

**Overall Phase:** The project is currently in the Phase-II development stage.
**Completed Work:** Project requirements, system architecture, component planning, and DSA technique selection are finalized.
**Inverted Index Data Structure:** In Progress.
**Text Preprocessing:** In Progress.
**Document Indexing:** In Progress.
**Search Functionality:** In Progress.
**Testing and Validation:** In Progress (basic functionality tested with sample documents).
**Performance Optimization:** Pending (to be executed after core functionality is complete).   




<br>
<br>
# Inverted-Index-and-Code-Based-Search-Engine

In today’s digital world, the amount of information stored in files, documents, source-code repositories, and databases is increasing rapidly. Finding relevant information from a large collection of files using traditional methods can be slow because every search may require scanning files one by one. This is especially challenging for large codebases, where developers need to quickly locate functions, variables, classes, keywords, or specific code patterns. Therefore, an efficient search mechanism is
required to retrieve relevant information quickly. 

The Inverted Index and Code-Based Search Engine aims to develop a fast and efficient search system for text and source-code files. An inverted index maps keywords to the files and locations where they occur. Instead of scanning every file for each query, the system directly accesses the index, significantly reducing search time.

The project focuses on applying Data Structures and Algorithms concepts to build a practical search engine. It can use structures such as hash tables for indexing and fast keyword lookup, linked lists for maintaining document references, trees for organized searching, and sorting and searching algorithms for ranking and retrieving results. For source-code files, the system can additionally identify programming-specific elements such as functions, variables, classes, and keywords.

The main problem addressed by this project is therefore how to efficiently search and retrieve relevant information from a large collection of text and code files while minimizing search time and unnecessary file scanning. The proposed system aims to provide faster keyword-based searching, organized results, and a foundation for scalable code-search applications. It also demonstrates how fundamental DSA concepts can be combined to solve a real-world information retrieval problem.
