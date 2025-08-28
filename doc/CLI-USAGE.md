# Command Line Usage

[TOC]

This document describes in detail the various ways in which you can interact with the `grader` and `profgrader` tool on the command line. 


## Notation

<!-- FIXME: Highlighted commands, `$` esspecially, would be nice -->

Unless otherwise specified, anything prefixed with a `$` character in a code block is a command, and all other content is command output. `...` may be used to snip irrelevant output for brevity.

Most flags have both short and long forms. For instance, the flag to get the program version may be specified by either `-V` or `--version`. This document will mostly provide flags in their long form, as their purpose is always more clear that way.

> [!NOTE]
> All of the command syntax assumes that you have [added the executable to your path](#adding_to_path), which is highly recommended. If you have not, make sure you prefix all invocations with the binary's qualified path, like in the following example, where `grader` is located in the `~` directory:
> ```command
> $ ~/grader lab1-2
> ...
> ```

## Students Only

> [!CAUTION]
> Coming soon...
> Read the output of `--help` for now

## Professor Only

> [!CAUTION]
> Coming soon...
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
 - Professor mode is generally one level "quieter" than student mode for the output of each individual student
 - Each level includes all output from levels below, possibly formatted a little differently
   e.g., Setting the verbosity to `FailedOnly` results in all output described in:
        `FailedOnly`, `Summary`, and `Quiet` (though not the retcode from `Silent`)

##### Silent {#verbosity_levels_silent_desc}

- **Student** - Nothing on stdout. Program return code = # of failed tests
- **Professor** - Nothing on stdout. Program return code = # of students not passing all tests

##### Quiet {#verbosity_levels_quiet_desc}

- **Student** - Overall test results only (# of tests and requirements passed/failed)
- **Professor** - Overall statistics only (# of students passed/failed, etc.)

##### Summary (default) {#verbosity_levels_summary_desc}

- **Student** - Lists the names of all tests, but only failing requirements
- **Professor** - Pass / fail stats for each individual student (like `Quiet` in student mode)

##### FailsOnly {#verbosity_levels_failsonly_desc}

- **Student** - Same as `Summary`
- **Professor** - Lists the names of all tests, but only failing requirements for each individual student

##### All {#verbosity_levels_all_desc}
- **Student** - Lists the result of all tests and requirements
- **Professor**  - \\\\ for each individual student


##### Extra {#verbosity_levels_extra_desc}
  
- **Student** - (Not yet implemented)
- **Professor** - (Not yet implemented)

##### Max {#verbosity_levels_max_desc}
- **Student** - Same as `Extra`
- **Professor** - \\\\

## Adding to PATH {#adding_to_path}

Navigate to the directory where you downloaded the grader executable, then run the following commands:

```command
$ mkdir -p ~/bin
$ mv grader ~/bin
$ echo "export PATH=\"$PATH:~/bin\"" >> ~/.bashrc
```
