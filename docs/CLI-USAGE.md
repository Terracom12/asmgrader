# Command Line Usage

[TOC]

This document describes in detail the various ways in which you can interact with the `grader` and `profgrader` tool on the command line. 


## Notation

<!-- FIXME: Highlighted commands, `$` especially, would be nice -->

Unless otherwise specified, anything prefixed with a `$` character in a code block is a command, and all other content is command output. `...` may be used to snip irrelevant output for brevity. `#` is used as an inline comment, and is neither a command nor output.

Most flags have both short and long forms. For instance, the flag to get the program version may be specified by either `-V` (capital `V`) or `--version`. This document will mostly provide flags in their long form, as their purpose is always more clear that way.

> [!NOTE]
> All of the command syntax assumes that you have [added the executable to your path](#adding_to_path), which is highly recommended. If you have not, make sure you prefix all invocations with the binary's qualified path, like in the following example, where `grader` is located in the `~` directory:
> ```command
> $ ~/grader lab1-2
> ...
> ```

## Students Only

Example usage:

```command
$ grader lab1-2
```

> [!TIP]
> More coming soon...
> Read the output of `--help` for now

## Professor Only

> [!WARNING]
> Behavior is undefined if multiple students have the same first and last names.

Example usage:

```command
$ profgrader lab1-2 --search-path ~/Documents/lab1-2-submissions
```

### Short Form Output

For less verbose output to just get a list of student scores, invoke with the `-q`|`--quiet` flag.

```console
$ profgrader lab1-2 -q
Jane Doe: Output score 100.00% (7/7 points)
John Doe: Output score 71.43% (5/7 points)
Alice Liddell: Could not locate executable
Bob Roberts: Output score 57.14% (4/7 points
```

### Student Database

The student database contains a list of students' first and last names, and is used to aid in locating student lab submissions as well as to provide diagnostics for students with a missing submission. It is recommended to provide a student database; see [not providing a database](#not_providing_a_database) for more info.

By default, a database named `students.csv` in the current working directory will be used if it exists. To specify a database manually, provide the `--database` argument, like as follows:

```command
$ profgrader lab1-2 --database ~/my-custom-database.csv
```

#### Not Providing a Database {#not_providing_a_database}

Without a database, `profgrader` will search for submissions according to the specified assignment and file matcher RegEx. By default, the searching behavior is essentially identical to if a database had been provided, except that a student name may be matched by any sequence of alphabetic characters.

For example, the following invocation of `profgrader` will only match the files `doejane_0000_0000_lab1-2.out` and `doejohn_0000_0000_lab1-2.out`.

```command
$ ls
doejohn_0000_0000_lab1-2.out    liddellalice_0000_0000_lab3-1.out  
doejane_0000_0000_lab1-2.out    robertsbob_0000_0000_lab5-3.out  

$ profgrader lab1-2
```

> [!TIP]
> More coming soon...
> Read the output of `--help` for now


## Advanced Usage

For details on all supported arguments, use the `--help` flag.

```command
$ grader --help
...
```

To check what version of the library you have, use the `--version` flag. When possible, versioning adheres to the [semver spec](https://semver.org/). The field after `-g` is the hash of the latest git commit when built.

```command
$ grader --version
AsmGrader v0.0.0-g123456789abcdef
```

### Output Verbosity Level
To increase or decrease the level of output verbosity use the `-v`|`--verbose` or `-q`|`--quiet` flags, respectively. These flags may be repeated. `--silent` is an alias for the minimum level of verbosity.
```command
$ grader <lab-name> --verbose
...
$ grader <lab-name> --quiet
...
$ grader <lab-name> -vvv
...
$ grader <lab-name> -qq
...

# etc. You get the idea

$ grader <lab-name> --silent
<no output>
```


#### Description of Levels {#verbosity_levels_desc}
 - Each level includes all output from levels below, possibly formatted a little differently
   e.g., Setting the verbosity to `All` results in all output described in:
        `Summary`, and `Quiet` (though not the retcode from `Silent`)

##### Silent {#verbosity_levels_silent_desc}

- **Student** - Nothing on stdout. Program return code = # of failed tests
- **Professor** - Nothing on stdout. Program return code = # of students not passing all tests

##### Quiet {#verbosity_levels_quiet_desc}

- **Student** - Overall test results only (# of tests and requirements passed/failed)
![](verbosity-quiet-student-screenshot.png)
- **Professor** - Per-student score only (percentage and number of tests passed + failed)
![](verbosity-quiet-prof-screenshot.png)

##### Summary (default) {#verbosity_levels_summary_desc}

![](verbosity-summary-screenshot.png)
- **Student** - Lists the names of all tests, but only failing requirements. If necessary, a brief diagnostic is displayed showing why a requirement failed
- **Professor**  - Same as above, but for each individual student

##### All {#verbosity_levels_all_desc}

![](verbosity-all-screenshot.png)
- **Student** - Lists the result of all tests and requirements. More verbose diagnostics for both failing and passing tests are shown.
- **Professor**  - Same as above, but for each individual student

##### Extra {#verbosity_levels_extra_desc}

(Not yet implemented)

##### Max {#verbosity_levels_max_desc}

(Not yet implemented)

## Adding to PATH {#adding_to_path}

Navigate to the directory where you downloaded the grader executable, then run the following commands:

```command
$ mkdir -p ~/bin
$ mv grader ~/bin
$ echo "export PATH=\"$PATH:~/bin\"" >> ~/.bashrc
```
