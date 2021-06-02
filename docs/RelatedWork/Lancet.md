[Doc Link](https://drive.xttech.tech/s/QALozdk6yykKrXD)

<p style="color: red; font-weight: bold">>>>>>  gd2md-html alert:  ERRORs: 0; WARNINGs: 0; ALERTS: 1.</p>
<ul style="color: red; font-weight: bold"><li>See top comment block for details on ERRORs and WARNINGs. <li>In the converted Markdown or HTML, search for inline alerts that start with >>>>>  gd2md-html alert:  for specific instances that need correction.</ul>

<p style="color: red; font-weight: bold">Links to alert messages:</p><a href="#gdcalert1">alert1</a>

<p style="color: red; font-weight: bold">>>>>> PLEASE check and correct alert issues and delete this message and the inline alerts.<hr></p>


What Lancet is:



*   “Self-correcting latency measurement tool”
*   “Relies on online statistical tests”

Background information:



*   Statistical Analysis
    *   Hypothesis Testing
    *   Anderson Darling Test
    *   Augmented Dickey Fuller Test
    *   Spearman Rank
    *   Independent and Identically Distributed Data
    *   Confidence Intervals
*   Packet/RPC Generators
    *   Packet Generators categorize tools that measure network devices and/or functions
        *   These “use stateless network packets to measure the throughput and the latency of datacenter network equipment such as switches and routers as defined in RFCs”
    *   RPC generators categorize tools that measure servers
        *   “measure the latency of client-server interactions using protocols such as http or memcached’s binary protocol, typically implemented on top of TCP or RDMA connections”
*   Symmetrical vs Asymmetrical generator
    *   Symmetrical generators
        *   These generate load and measure latency at the same time 
        *   An external agent will gather these measurements and process them
    *   Asymmetrical Generator
        *   These have separate clients for load generation and latency-measuring
        *   Load generation happens in an open loop manner
        *   Latency measuring happens in a closed loop manner on a separate dedicated client/machine
        *   Reduces system load on latency generator and thus reduce client bias

Lancet’s Design and Implementation:



*   A distributed tool
    *   Consists of a Coordinator and various measuring agents
        *   Coordinator “in charge of the experiment methodology (see §4.3) and communicates with the agents over the LANCET API”
        *   Measuring agents “drive the workload via application RPCs generated based on application-specific random distributions”
            *   “measure latency precisely, identify cases of workload violations, and run statistical tests”
    *   

<p id="gdcalert1" ><span style="color: red; font-weight: bold">>>>>>  gd2md-html alert: inline image link here (to images/image1.png). Store image on your image server and adjust path/filename/extension if necessary. </span><br>(<a href="#">Back to top</a>)(<a href="#gdcalert2">Next alert</a>)<br><span style="color: red; font-weight: bold">>>>>> </span></p>


![alt_text](images/image1.png "image_tooltip")

    *   Self Correction
        *   This functionality tries to identify:
            *   When the server is stable enough to start measuring latency
            *   When latency samples converge and if tail phenomena has been sufficiently covered
            *   Process samples and report latency without statistical bias
            *   Confidence intervals of results
        *   System stability
            *   Ensures target load has been achieved in the system before measurements
                *   This avoids transient phenomena
            *   “ensures that agents load the server while respecting the workload’s specified inter-arrival distribution”
                *   Avoids misleading latency is not reported
        *   Unbiased result processing
            *   Agents collect a specified number of random samples at a specified rate
            *   Must use sampling because they need samples to be independent and identically distributed (iid) in order to calculate confidence intervals correctly
            *   They know the samples are iid or not by “computing the autocorrelation of the collected latency samples sorted by their transmission time”
                *   Does this by computing “the Spearman correlation of the collected latency samples shifted over time.”
        *   Result stationarity
            *   This means that they want to know whether or not the samples collected is “sufficient for the results to converge to a stable distribution of latencies that does not change over time”
            *   This is achieved by using an Augmented Dickey Fuller test
        *   Termination
            *   Finished when target confidence is reached 
                *   “reports the final latency percentiles with the equivalent confidence intervals”
            *   If not, then experiment finishes when retries limit is exceeded or experiment runs too long 
                *   Reports inconclusive results, why it’s inconclusive and what has been gathered
    *   Overall Software implementation
        *   A coordinator
            *   Deploys agents, communicates with agents, drives state machine