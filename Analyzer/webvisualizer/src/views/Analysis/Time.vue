<template>
  <v-container>
    <v-row>
      <counting-view :jobid="jobid"></counting-view>
    </v-row>
     <v-row>
      <timing-view :jobid="jobid"></timing-view>
    </v-row>
  </v-container>
</template>

<script>
import CountingView from "@/views/Analysis/TimeComponents/CountingView.vue";
import TimingView from "@/views/Analysis/TimeComponents/TimingView.vue";

export default {
  components: {CountingView, TimingView},
  props: ["jobid"],
  model: {},
  mounted: function () {
  },
};
</script>
