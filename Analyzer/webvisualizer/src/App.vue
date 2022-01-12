<template>
  <v-app>
     <v-navigation-drawer
      v-model="drawer"
      app
    >
    </v-navigation-drawer>
    
    <v-app-bar
      app
      color="primary"
      dark
    >
      <v-app-bar-nav-icon @click="drawer = !drawer"></v-app-bar-nav-icon>

      <v-toolbar-title>{{componentDisplayName}}</v-toolbar-title>

    </v-app-bar>

    <v-main>
       <router-view @updateDisplayName="componentDisplayName=$event"/>
    </v-main>
  </v-app>
</template>

<script>

export default {
  name: 'App',

  components: {
  },

  data: () => ({
    drawer:null,
    componentDisplayName:""
  }),

  methods:{

  },
  mounted() {
    console.log("Change display name")
  }

};

</script>

<style lang="scss">

</style>
