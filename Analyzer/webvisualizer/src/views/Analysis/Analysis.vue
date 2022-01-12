<template>
  <div class="home">
    <v-navigation-drawer v-model="drawer" :permanent="permanent" clipped app>
      <v-list-item>
        <v-list-item-content>
          <v-list-item-title class="text-h6"> Application </v-list-item-title>
          <v-list-item-subtitle> subtext </v-list-item-subtitle>
        </v-list-item-content>
      </v-list-item>
      <v-divider></v-divider>

      <v-list dense nav>
        <v-list-item v-for="item in items" :key="item.title" link>
          <v-list-item-icon>
            <v-icon>{{ item.icon }}</v-icon>
          </v-list-item-icon>

          <v-list-item-content>
            <v-list-item-title>{{ item.title }}</v-list-item-title>
          </v-list-item-content>
        </v-list-item>
      </v-list>
    </v-navigation-drawer>

  </div>
</template>

<script>
export default {
  name: "Home",
  components: {
    // Welcome,
  },
  prop: {},
  model: {},
  data: {},
  beforeMount() {
    this.$emit("updateDisplayName", "Welcome");
  },
};
</script>
