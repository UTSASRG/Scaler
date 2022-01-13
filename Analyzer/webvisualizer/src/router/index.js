import Vue from 'vue'
import VueRouter from 'vue-router'

Vue.use(VueRouter)

const routes = [
  {
    path: '/',
    name: 'Home',
    redirect: '/about'
  },
  {
    path: '/about',
    name: 'About',
    component: () => import('@/views/About/About.vue')

  },
  {
    path: '/analysis',
    name: 'Analysis',
    component: () => import('@/views/Analysis/About.vue')
  },
  {
    path: '/analysis/:id',
    name: 'Analysis',
    component: () => import('@/views/Analysis/Analysis.vue'),
    props: true,
    children: [
      {
        path: 'execution',
        component: () => import('@/views/Analysis/Components/Execution.vue')
      },
      {
        path: 'symbol',
        component: () => import('@/views/Analysis/Components/Symbol.vue')
      },
      {
        path: 'time',
        component: () => import('@/views/Analysis/Components/Time.vue')
      },
      {
        path: 'callgraph',
        component: () => import('@/views/Analysis/Components/CallGraph.vue')
      },
      {
        path: '',
        redirect: '/analysis'
      }
    ]
  },
  {
    path: '/run',
    name: 'Run',
    component: () => import('@/views/Run/About.vue')
  },
  {
    path: '/run/:id',
    name: 'Run',
    component: () => import('@/views/Run/Run.vue'),
    props: true
  },
  {
    path: '*',
    name: '404 Error',
    component: () => import('@/views/Errors/404.vue')
  },
]

const router = new VueRouter({
  mode: 'history',
  base: process.env.BASE_URL,
  routes
})

export default router
