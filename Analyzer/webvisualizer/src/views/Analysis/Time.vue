<template>
  <v-container>
     <v-row>
      <timing-view :jobid="jobid"></timing-view>
    </v-row>
  </v-container>
</template>

<script>
// import CountingView from "@/views/Analysis/TimeComponents/CountingView.vue";
import TimingView from "@/views/Analysis/TimeComponents/TimingView.vue";

export default {
  components: {TimingView},
  props: ["jobid"],
  model: {},
  mounted: function () {
  },
};
</script>
