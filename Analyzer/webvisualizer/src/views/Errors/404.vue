<template>
  <v-container fill-height fluid>
    <v-row align="center" justify="center">
      <v-col class="text-center">
        <div class="text-h3">404 not found</div>
      </v-col>
    </v-row>
  </v-container>
</template>

<script>
export default {
  name: "404 Error",
  components: {},
  prop: {},
  model: {},
   beforeMount() {
    this.$emit("updateDisplayName", "");
  },
};
</script>
