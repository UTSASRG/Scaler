Steps Taken for Perf Record and Data Parsing

<ol>
<li> /usr/bin/perf record --freq=997 --call-graph dwarf -q -o [Output file] [command to record or executable to record] (can also try perf record without having to input the file path) </li>
  <ol>
    <li> VERY IMPORTANT: </li>
      <ol>
      <li> sudo sh -c &#39;echo 1 \&gt; /proc/sys/kernel/perf\_event\_paranoid&#39; </li>
      <li> sudo sh -c &#39;echo 0 \&gt; /proc/sys/kernel/kptr\_restrict&#39; </li>
      <li> These command should be ran in order to allow perf record to run otherwise perf record will fail due to not having the correct permissions
      </ol>
    <li> -q is not necessary (q as in quiet, it simply silently removes any output that perf record sends to the terminal, this may be errors or minor warnings), can remove it and it removing it may help with solving problems involving perf record as it will output to the terminal any recording errors </li>
    <li> --call-graph dwarf is necessary in order to get function stacks, this tells perf record to retrieve call graphs when sampling. </li>
      <ol>
      <li> The dwarf option tells perf record to copy from stack memory, which is much more reliable but more costly to get call graphs </li>
      </ol>
    <li> --freq=997 simply sets the desired target sampling frequency to 997 Hz. </li>
    <li> -o [output file] tells perf record to write what it records into the output file we specified in [output file], otherwise it will create a default file called perf.data, (I believe it is created in /tmp/, not certain though) </li>
    <li> Perf record will actually run the command or executable it tell it in order to sample it. </li>
      <ol>
        <li> This creates some complications for commands or executables that are alreadying running, there is a --pid option that lets you tell perf record to record a running application, but with memcached, I couldn&#39;t get it to work, thus I took a different path explained in the next line </li>
      </ol>
    <li> Recording memcached is a little tricky, I had to actually kill the memcached process that is automatically started and then run the same command that started the process via perf record. </li>
    <ol>
      <li> While it was running, I then ran the memcached benchmark memaslap at default settings for 1 minute and then I had to forcefully kill or ctrl+c terminate perf record at which point I could get the perf data file. </li>
    </ol>
  </ol>
<li> Perf Script --i [input file] \&gt; [output file] </li>
  <ol>
    <li> Here I simply ran perf script with default settings aside from the --i option which allows me to specify an input file to run perf script on where if I don&#39;t specify this, perf script will try to operate on the default perf.data file that perf record will make if -o is not used. </li>
    <li> Perf script simply parses and dumps the data from the output file of perf record into a text format which can be parsed. The reason this is necessary is because the output data file from perf record is a binary file, so it is difficult to understand without converting it into a text format with this command </li>
  </ol>
<li> Perl Script from Brandon Gregg&#39;s Flame Graph Tool: Stackcollapse-perf.pl with --tid and --all options </li>
  <ol>
    <li> This script will reduce the perf script output into the information we care about the most, the command and tid of each call stack, each function call for each call stack and the amount of samples recorded for each call stack </li>
    <li> I have an edited version of this script that will report the dso or shared object for a called function for all functions except for when [unknown] is reported or \_[k], \_[j], or \_[i] is reported (These suffixes are tacked on when -all is used with the script) </li>
  </ol>
<li> Past this point, I take the output of the perl script and run it through my own data aggregator python script which is documented on the script itself </li>
</ol>