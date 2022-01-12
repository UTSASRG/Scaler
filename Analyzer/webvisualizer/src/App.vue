<template>
  <v-app>
    <v-app-bar app clipped-left>
      <v-toolbar-title class="overflow-visible">
        <v-img
          :src="require('./assets/logo.svg')"
          class="my-3"
          contain
          width="80px"
      /></v-toolbar-title>
      <v-tabs align-with-title v-model="active_tab">
        <v-tab v-for="item in mainMenuItems" :to="item.path" :key="item.id">{{
          item.name
        }}</v-tab>
      </v-tabs>
    </v-app-bar>

    <v-main>
      <router-view
        class="subcomponentview"
        @updateDisplayName="componentDisplayName = $event"
      />
    </v-main>
  </v-app>
</template>

<script>
export default {
  name: "App",

  components: {},

  data: () => ({
    drawer: null,
    active_tab: 2,
    mainMenuItems: [
      { id: 0, name: "Run", path: "/run" },
      { id: 1, name: "Visualize", path: "/visualize" },
      { id: 2, name: "About", path: "/about" },
    ],
  }),
  methods: {},
  computed:{
   
  },
  mounted() {
    console.log("Change display name");
  },
};
</script>

<style lang="css">
html {
  overflow-y: auto;
}
body {
  margin: 0;
  padding: 0;
  /* Hide scrollbar */
  scrollbar-width: none;
}
body::-webkit-scrollbar {
  /* Hide scrollbar */
  display: none;
}
.subcomponentview {
  scrollbar-width: auto;
}
</style>
