<template>
  <v-container>
    <v-row>
      <v-col>
        <p class="text-h4">What is scaler?</p>
        <p class="text--primary">
          Scaler is a tool to holistically pinpoint scalability bottleneck in
          whole system stack.
        </p>
        <p class="text--primary">
          Scaler is composed of the following components:
        </p>
      </v-col>
    </v-row>
    <v-row>
      <v-col
        v-for="coreCompDesc in coreCompDescs"
        :key="coreCompDesc.name"
        class="d-flex"
      >
        <v-card class="mx-auto d-flex flex-column" min-width="344" max-width="344">
          <v-card-text>
            <div>{{ coreCompDesc.role }}</div>
            <div class="d-flex">
              <p class="text-h5 text--primary">
                {{ coreCompDesc.name }}
              </p>
              <v-chip
                v-if="coreCompDesc.deprecated"
                class="ma-2 borderless"
                x-small
                color="primary"
              >
                deprecated
              </v-chip>
            </div>
            <div class="text--primary">{{ coreCompDesc.desc }}.<br /></div>
          </v-card-text>
           <v-spacer></v-spacer>
          <v-card-actions>
            <v-btn text :href="coreCompDesc.url" target="_blank">
              Source
            </v-btn>
          </v-card-actions>
        </v-card>
      </v-col>
    </v-row>
  </v-container>
</template>

<script>
export default {
  data: () => ({
    coreCompDescs: [
      {
        name: "ScalerHook",
        role: "Profiler",
        desc: "A library that performs profiling and records profiling data",
        url: "https://github.com/UTSASRG/Scaler/tree/feature-analyzeScaleroutput/libHook",
      },
      {
        name: "ScalerWeb",
        role: "Analysis, Configurator",
        desc: "Frontend for scaler. A comprehensive visualization, analysis and configurator for Scaler",
        url: "https://github.com/UTSASRG/Scaler/tree/feature-analyzeScaleroutput/Analyzer/webvisualizer",
      },
      {
        name: "ScalerRun",
        role: "Configurator",
        desc: "A convenient tool to run scaler. It controls scaler behavior through commandline options or WebVisualizer",
        url: "https://github.com/UTSASRG/Scaler/tree/feature-analyzeScaleroutput/Analyzer/ScalerRun",
        deprecated: false,
      },
      {
        name: "PyVisualizer",
        role: "Visualization",
        desc: "A python-written visualizer for profiling data visualization. Supports both scaler format and perf format",
        url: "https://github.com/UTSASRG/Scaler/tree/feature-analyzeScaleroutput/Analyzer/PyVisualizer",
        deprecated: true,
      },
    ],
  }),
};
</script>
