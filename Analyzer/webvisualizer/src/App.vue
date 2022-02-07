<template>
  <v-app>
    <v-app-bar app clipped-left>
      <v-toolbar-title class="overflow-visible">
        <v-img
          :src="require('@/assets/imgs/logo.svg')"
          class="my-3"
          contain
          width="80px"
        />
      </v-toolbar-title>

      <v-tabs v-if="!showReducedMainMenu" align-with-title v-model="active_tab">
        <v-tab v-for="item in mainMenuItems" :to="item.path" :key="item.id">{{
          item.name
        }}</v-tab>
      </v-tabs>
      <v-spacer />
      <div v-if="showReducedMainMenu" class="text-h6">
        {{ componentDisplayName }}
      </div>
      <v-spacer />
      <v-menu v-if="showReducedMainMenu">
        <template v-slot:activator="{ on, attrs }">
          <v-btn icon v-bind="attrs" v-on="on" class="right">
            <v-icon>mdi-dots-vertical</v-icon>
          </v-btn>
        </template>
        <v-list>
          <v-list-item
            v-for="item in mainMenuItems"
            :key="item.id"
            @click="
              () => {
                $router.push({ path: item.path });
              }
            "
          >
            <v-list-item-title>{{ item.name }}</v-list-item-title>
          </v-list-item>
        </v-list>
      </v-menu>
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
  name: "Test",
  components: {},
  data: () => ({
    drawer: null,
    active_tab: 2,
    componentDisplayName: "",
    mainMenuItems: [
      { id: 0, name: "Run", path: "/run" },
      { id: 1, name: "Analysis", path: "/analysis" },
      { id: 2, name: "About", path: "/about" },
    ],
  }),
  methods: {},
  computed: {
    showReducedMainMenu() {
      switch (this.$vuetify.breakpoint.name) {
        case "xs":
          return true;
        case "sm":
          return false;
        case "md":
          return false;
        case "lg":
          return false;
        case "xl":
          return false;
      }
      return true;
    },
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
