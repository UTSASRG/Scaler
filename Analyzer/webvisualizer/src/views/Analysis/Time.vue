<template>
  <v-container>
    <v-row>
      <counting-view :jobid="jobid"></counting-view>
    </v-row>
  </v-container>
</template>

<script>
import CountingView from "@/views/Analysis/TimeComponents/CountingView.vue";

export default {
  components: {CountingView },
  props: ["jobid"],
  model: {},
  mounted: function () {
  },
};
</script>
