<template>
  <v-container>
    <v-row>
      <v-subheader class="text-h4">Config Editor</v-subheader>
    </v-row>
    <v-row>
      <v-col>
        <v-textarea auto-grow v-model="configYaml"> </v-textarea>
      </v-col>
    </v-row>
    <v-row>
      <v-col>
        <div>
          <v-btn color="primary" @click="execute()"> Execute </v-btn>
          <v-btn color="error" @click="abort()"> Abort </v-btn>
        </div>
      </v-col>
    </v-row>
  </v-container>
</template>

<script>
import axios from "axios";
import { scalerConfig } from "@/scalerconfig.js";

export default {
  name: "About",
  components: {},
  props: ["jobid"],
  model: {},
  methods: {
    execute: function () {
    let thiz = this;

      axios
      .post(
        scalerConfig.$ANALYZER_SERVER_URL +
          "/config/yamlconfig?jobid=" +
          thiz.jobid,
          { 'yamlConfig': thiz.configYaml }
      )
      /*.then(function (response) {
        thiz.configYaml=response.data
      }
      )*/;
    },
    abort: function () {
    let thiz = this;
      axios
      .get(
        scalerConfig.$ANALYZER_SERVER_URL +
          "/config/yamlconfig/cancel?jobid=" +
          thiz.jobid
      )
      /*.then(function (response) {
        thiz.configYaml=response.data
      }
      )*/;
    },
  },
  data: () => {
    return {
      configYaml: null,
    }
  },
  beforeMount() {
    this.$emit("updateDisplayName", "Run");
  },
  mounted: function () {
    let thiz = this;
    //Getting ELFimgInfo from the server
    axios
      .get(
        scalerConfig.$ANALYZER_SERVER_URL +
          "/config/yamlconfig?jobid=" +
          this.jobid
      )
      .then(function (response) {
        thiz.configYaml=response.data
      });
  },
};
</script>
