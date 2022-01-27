<template>
  <v-container>
    <v-navigation-drawer v-model="drawer" permanent expand-on-hover clipped app mini-variant>
      <v-list-item>
        <v-list-item-content>
          <v-list-item-title class="text-h6">Analyzing</v-list-item-title>
          <v-list-item-subtitle>ID={{ jobid }}</v-list-item-subtitle>
        </v-list-item-content>
      </v-list-item>
      <v-divider></v-divider>

      <v-list dense nav>
        <v-list-item
          v-for="item in menuItems"
          :key="item.id"
          link
          :href="itemIdPath(id, item.path)"
        >
          <v-list-item-icon>
            <v-icon>{{ item.icon }}</v-icon>
          </v-list-item-icon>

          <v-list-item-content>
            <v-list-item-title>{{ item.title }}</v-list-item-title>
          </v-list-item-content>
        </v-list-item>
      </v-list>
    </v-navigation-drawer>
    <router-view :jobid="jobid"> </router-view>
  </v-container>
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
        path: "execution",
      },
      {
        id: 1,
        title: "Symbols",
        icon: "mdi-function",
        path: "symbol",
      },
      {
        id: 2,
        title: "Time analysis",
        icon: "mdi-chart-timeline-variant-shimmer",
        path: "time",
      },
      {
        id: 3,
        title: "Call graph",
        icon: "mdi-graph",
        path: "callgraph",
      },
    ],
  }),
  methods: {
    itemIdPath: function(id, itemPath) {
      return "/analysis/"+id + "/" + itemPath;
    },
  },
  props: ["jobid","id"],
  model: {},
  beforeMount() {
    this.$emit("updateDisplayName", "Analysis");
  },
};
</script>
