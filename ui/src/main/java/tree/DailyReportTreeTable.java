package tree;

import data.Task;
import data.TimeData;
import net.byteseek.swing.treetable.TreeTableHeaderRenderer;
import net.byteseek.swing.treetable.TreeTableModel;
import net.byteseek.swing.treetable.TreeUtils;
import packets.DailyReportMessage;
import packets.TaskInfo;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.MutableTreeNode;
import javax.swing.tree.TreeNode;
import java.awt.*;
import java.time.Instant;
import java.util.*;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.function.Predicate;

import static taskglacier.MainFrame.mainFrame;

public class DailyReportTreeTable extends JTable {
    private final DefaultMutableTreeNode rootNode = new DefaultMutableTreeNode(new Task(0, 0, ""), true);
    private final TreeTableModel treeTableModel;
    private final DefaultTreeModel treeModel;

    private Map<TimeData.TimeEntry, DailyReportTreeTableModel.CategoryNode> categoryNodes = new HashMap<>();

    // we'll use this to decide which nodes to delete, if any, later
    Map<TimeData.TimeEntry, Set<Task>> tasks = new HashMap<>();

    Map<Integer, Integer> parents = new HashMap<>();

    public DailyReportTreeTable() {

        treeTableModel = createTreeTableModel(rootNode);
        treeModel = createTreeModel(rootNode);

        treeTableModel.setNodeFilter(treeNode -> false);

        getColumnModel().getColumn(1).setCellRenderer(new ElapsedTimeCellRenderer());

        setIntercellSpacing(new Dimension(0, 0));
    }

    public void update(DailyReportMessage.DailyReport report) {


        List<Integer> taskIDs = report.times.stream()
                .map(timePair -> timePair.taskID)
                .toList();

        boolean parentsChanged = false;

        for (int taskID : taskIDs) {
            Task task = mainFrame.getTaskModel().getTask(taskID);

            Integer parent = parents.get(task.id);

            if (parent != null && parent.intValue() != task.parentID) {
                parentsChanged = true;
                break;
            }
        }

        // if parents have changed, let's throw out all the task nodes and start over
        if (parentsChanged) {
            System.out.println("Parents changed, delete all tasks from display");
            for (DailyReportTreeTableModel.CategoryNode value : categoryNodes.values()) {
                Enumeration<TreeNode> children = value.children();
                while (children.hasMoreElements()) {
                    treeModel.removeNodeFromParent((MutableTreeNode) children.nextElement());
                }
            }
            tasks.clear();
        }

        parents.clear();

        for (int taskID : taskIDs) {
            Task task = mainFrame.getTaskModel().getTask(taskID);
            parents.put(task.id, task.parentID);
            System.out.printf("task %d parent %d%n", task.id, task.parentID);
        }

        addNewCategoryNodes(report);
        removeUnusedCategoryNodes(report);

        // we'll use this to decide which nodes to delete, if any, later
        Map<TimeData.TimeEntry, Set<Task>> tasksThisUpdate = new HashMap<>();

        for (Integer taskID : taskIDs) {
            List<DailyReportMessage.DailyReport.TimePair> pairs = report.times.stream()
                    .filter(timePair -> timePair.taskID == taskID)
                    .toList();

            Task task = mainFrame.getTaskModel().getTask(taskID);

            for (DailyReportMessage.DailyReport.TimePair pair : pairs) {
                TaskInfo.Session session = task.sessions.get(pair.index);

                Instant stopTime = session.stopTime.orElseGet(() -> report.time);
                Instant instant = stopTime.minusMillis(session.startTime.toEpochMilli());
                long minutes = TimeUnit.MILLISECONDS.toMinutes(instant.toEpochMilli());

                for (TimeData.TimeEntry timeEntry : session.timeEntry) {
                    Optional<DailyReportTreeTableModel.CategoryNode> list = categoryNodes.values().stream()
                            .filter(categoryNode -> categoryNode.category.equals(timeEntry.category) && categoryNode.code.equals(timeEntry.code))
                            .findFirst();

                    if (list.isPresent()) {
                        Set<Task> orDefault = tasks.getOrDefault(timeEntry, new HashSet<>());
                        tasksThisUpdate.put(timeEntry, orDefault);
                        orDefault.add(task);

                        DailyReportTreeTableModel.TaskNode taskNode = findOrCreateTaskNode(list.get(), task);
                        taskNode.minutes = minutes;
                    }
                }
            }
        }

        tasks.forEach((timeEntry, tasks1) -> {
            Set<Task> thisUpdate = tasksThisUpdate.get(timeEntry);

            if (thisUpdate != null) {
                DailyReportTreeTableModel.CategoryNode categoryNode = categoryNodes.get(timeEntry);

                for (Task task : tasks1) {
                    if (!thisUpdate.contains(task)) {
                        // remove
                        treeModel.removeNodeFromParent(findOrCreateTaskNode(categoryNode, task));
                    }
                }
            }
        });

        this.tasks = tasksThisUpdate;
    }

    private DailyReportTreeTableModel.TaskNode findOrCreateTaskNode(DailyReportTreeTableModel.CategoryNode categoryNode, Task task) {
        // first, jump to the highest non-0 parent
        List<Task> history = new ArrayList<>();
        history.add(task);

        Task current = task;

        while (current.parent != null) {
            history.add(0, current.parent);
            current = current.parent;
        }

        DefaultMutableTreeNode node = categoryNode;
        for (Task task1 : history) {
            Enumeration<TreeNode> children = node.children();
            boolean found = false;
            while (children.hasMoreElements()) {
                DefaultMutableTreeNode child = (DefaultMutableTreeNode) children.nextElement();

                if (child instanceof DailyReportTreeTableModel.TaskNode taskNode) {
                    if (task1 == taskNode.task) {
                        // found it, loop again
                        node = child;
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                // didn't find it. create it
                DailyReportTreeTableModel.TaskNode newNode = new DailyReportTreeTableModel.TaskNode();
                newNode.task = task1;
                treeModel.insertNodeInto(newNode, node, 0);
                node = newNode;
            }
        }
        return (DailyReportTreeTableModel.TaskNode) node;
    }

    private void addNewCategoryNodes(DailyReportMessage.DailyReport report) {
        report.timesPerTimeEntry.forEach((timeEntry, time) -> {
            long minutes = TimeUnit.MILLISECONDS.toMinutes(time.toEpochMilli());

            // search for it first
            if (!categoryNodes.containsKey(timeEntry)) {
                DailyReportTreeTableModel.CategoryNode categoryNode = new DailyReportTreeTableModel.CategoryNode();
                categoryNode.category = timeEntry.category;
                categoryNode.code = timeEntry.code;
                categoryNode.minutes = minutes;

                categoryNode.setAllowsChildren(true);

                categoryNodes.put(timeEntry, categoryNode);
//                rootNode.add(categoryNode);
                treeModel.insertNodeInto(categoryNode, rootNode, 0);
            }
        });
    }

    private void removeUnusedCategoryNodes(DailyReportMessage.DailyReport report) {
        var iterator = categoryNodes.entrySet().iterator();

        while (iterator.hasNext()) {
            var next = iterator.next();

            if (!report.timesPerTimeEntry.keySet().contains(next.getKey())) {
                treeModel.removeNodeFromParent(next.getValue());
                iterator.remove();
            }
        }
    }

    private DefaultTreeModel createTreeModel(TreeNode rootNode) {
        DefaultTreeModel model = new DefaultTreeModel(rootNode);
        model.addTreeModelListener(treeTableModel);
        return model;
    }

    private TreeTableModel createTreeTableModel(TreeNode rootNode) {
        TreeTableModel localTreeTableModel = new DailyReportTreeTableModel(rootNode);

        TreeTableHeaderRenderer renderer = new TreeTableHeaderRenderer();
//        renderer.setShowNumber(true); // true is default, this code is just for testing the false option.

        localTreeTableModel.bindTable(this, renderer);
        localTreeTableModel.addExpandCollapseListener(new TreeTableModel.ExpandCollapseListener() {
            @Override
            public boolean nodeExpanding(TreeNode node) {
                if (node.getChildCount() == 0) { // if a node is expanding but has no children, set it to allow no children.
                    ((DefaultMutableTreeNode) node).setAllowsChildren(false);
                }
                return true;
            }

            @Override
            public boolean nodeCollapsing(TreeNode node) {
                return true;
            }
        });
        return localTreeTableModel;
    }
}
