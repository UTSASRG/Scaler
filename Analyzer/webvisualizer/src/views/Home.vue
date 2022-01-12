<template>
  <div class="home">
    <Welcome msg="Welcome to Your Vue.js App"/>
  </div>
</template>

<script>
// @ is an alias to /src
import Welcome from '@/components/Welcome.vue'

export default {
  name: 'Home',
  components: {
    Welcome
  },
  prop:{
  },
  model: {
  },
  mounted() {
    this.$emit('updateDisplayName','Welcome')
  }
}

 
</script>
