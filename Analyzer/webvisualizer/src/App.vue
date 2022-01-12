<template>
  <v-app>
    <v-app-bar app clipped-left>
      <v-toolbar-title>{{ componentDisplayName }}</v-toolbar-title>
    </v-app-bar>

    <v-main>
      <router-view class="subcomponentview" @updateDisplayName="componentDisplayName = $event" />
    </v-main>
  </v-app>
</template>

<script>
export default {
  name: "App",

  components: {},

  data: () => ({
    drawer: null,
    componentDisplayName: "",
    items: [
      { title: "Dashboard", icon: "mdi-view-dashboard" },
      { title: "Photos", icon: "mdi-image" },
      { title: "About", icon: "mdi-help-box" },
    ],
  }),

  methods: {},
  mounted() {
    console.log("Change display name");
  },
};
</script>

<style lang="css">
html {
  overflow: hidden;
  overflow-y: auto;
}
body {
  margin: 0;
  padding: 0;
  overflow: hidden;
  /* Hide scrollbar */
  scrollbar-width: none;
}
body::-webkit-scrollbar {
  /* Hide scrollbar */
  display: none; 
}
.subcomponentview{
  scrollbar-width: auto;
}

</style>
