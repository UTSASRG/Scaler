<template>
  <div class="home">
    <v-navigation-drawer v-model="drawer" permanent expand-on-hover clipped app>
      <v-list-item>
        <v-list-item-content>
          <v-list-item-title class="text-h6">Analyzing</v-list-item-title>
          <v-list-item-subtitle>ID={{ id }}</v-list-item-subtitle>
        </v-list-item-content>
      </v-list-item>
      <v-divider></v-divider>

      <v-list dense nav>
        <v-list-item v-for="item in menuItems" :key="item.id" link>
          <v-list-item-icon>
            <v-icon>{{ item.icon }}</v-icon>
          </v-list-item-icon>

          <v-list-item-content>
            <v-list-item-title>{{ item.title }}</v-list-item-title>
          </v-list-item-content>
        </v-list-item>
      </v-list>
    </v-navigation-drawer>
    Hello
  </div>
</template>

<script>
// @ is an alias to /src
// import Welcome from "@/components/Welcome.vue";

export default {
  name: "Home",
  components: {
    // Welcome,
  },
  data: () => ({
    drawer: null,
    menuItems: [
      {
        id: 0,
        title: "Execution",
        icon: "mdi-information-outline",
      },
      {
        id: 1,
        title: "Symbols",
        icon: "mdi-function",
      },
      {
        id: 2,
        title: "Time analysis",
        icon: "mdi-chart-timeline-variant-shimmer",
      },
      {
        id: 3,
        title: "Call graph",
        icon: "mdi-graph",
      },
    ],
  }),
  props: ["id"],
  model: {},
  beforeMount() {
    this.$emit("updateDisplayName", "Analysis");
  },
};
</script>
